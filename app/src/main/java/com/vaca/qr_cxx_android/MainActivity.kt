package com.vaca.qr_cxx_android

import android.Manifest
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.ImageFormat
import android.graphics.Matrix
import android.graphics.RectF
import android.hardware.camera2.*
import android.media.ImageReader
import android.net.Uri
import android.os.Bundle
import android.os.Handler
import android.os.HandlerThread
import android.provider.Settings
import android.util.Log
import android.util.Size
import android.view.Surface
import android.view.View
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import com.vaca.qr_cxx_android.ConvertUtils.convertNV21toJPEG
import com.vaca.qr_cxx_android.ConvertUtils.convertYUV_420_888toNV21
import com.vaca.qr_cxx_android.databinding.ActivityMainBinding
import java.io.File
import java.text.DecimalFormat


class MainActivity : AppCompatActivity() {
    external fun inputImage(b: ByteArray)
    external fun resetWorker()
    external fun stopWorker()
    external fun initWorker()
    external fun getProgress(): Int

    companion object {
        init {
            System.loadLibrary("qr_cxx_android")
        }
    }


    private val mCameraId = "0"
    lateinit var mPreviewSize: Size
    private val PREVIEW_WIDTH = 2048
    private val PREVIEW_HEIGHT = 2048
    private var mCameraDevice: CameraDevice? = null
    lateinit var mHandler: Handler
    lateinit var mCaptureSession: CameraCaptureSession
    lateinit var mPreviewBuilder: CaptureRequest.Builder
    private var mHandlerThread: HandlerThread? = null
    lateinit var mImageReader: ImageReader

    var haveConfig = false


    private fun startBackgroundThread() {
        mHandlerThread = HandlerThread("vaca")
        mHandlerThread!!.start()
        mHandler = Handler(mHandlerThread!!.looper)
    }


    private val mCameraDeviceStateCallback: CameraDevice.StateCallback =
        object : CameraDevice.StateCallback() {
            override fun onOpened(camera: CameraDevice) {
                mCameraDevice = camera
                startPreview(camera)
            }

            override fun onDisconnected(camera: CameraDevice) {
                camera.close()
            }

            override fun onError(camera: CameraDevice, error: Int) {
                camera.close()
            }

            override fun onClosed(camera: CameraDevice) {
                camera.close()
            }
        }

    private fun openCamera() {
        startBackgroundThread()
        try {
            val cameraManager =
                getSystemService(Context.CAMERA_SERVICE) as CameraManager
            mPreviewSize = Size(PREVIEW_WIDTH, PREVIEW_HEIGHT)
            cameraManager.openCamera(mCameraId, mCameraDeviceStateCallback, mHandler)
        } catch (e: CameraAccessException) {
            e.printStackTrace()
        }
    }


    private val mSessionStateCallback: CameraCaptureSession.StateCallback =
        object : CameraCaptureSession.StateCallback() {
            override fun onConfigured(session: CameraCaptureSession) {
                mCaptureSession = session
                updatePreview()
            }

            override fun onConfigureFailed(session: CameraCaptureSession) {}
        }

    private inner class ImageSaver(var reader: ImageReader) : Runnable {
        override fun run() {
            val image = reader.acquireLatestImage() ?: return
          //  Log.e("vaca","width:${image.width} height:${image.height}")
            val data = convertNV21toJPEG(
                convertYUV_420_888toNV21(image),
                image.width, image.height
            )
            if (!haveConfig) {
                haveConfig = true
                mPreviewSize = Size(image.width, image.height)
                configureTransform( binding.texture.width, binding.texture.height)
            }
            inputImage(data)
            image.close()
        }
    }


    private fun updatePreview() {
        mHandler.post {
            try {
                mCaptureSession.setRepeatingRequest(mPreviewBuilder.build(), null, mHandler)
            } catch (e: CameraAccessException) {
                e.printStackTrace()
            }
        }
    }

    private fun startPreview(camera: CameraDevice) {
        mPreviewBuilder = camera.createCaptureRequest(CameraDevice.TEMPLATE_RECORD)
        mImageReader = ImageReader.newInstance(
            mPreviewSize.width,
            mPreviewSize.height,
            ImageFormat.YUV_420_888,
            2
        )

        mPreviewBuilder.addTarget(mImageReader.surface)
        mImageReader.setOnImageAvailableListener(
            { reader ->
                mHandler.post(ImageSaver(reader))
            }, mHandler
        )

        val texture = binding.texture.surfaceTexture
        val surface = Surface(texture)

        mPreviewBuilder.addTarget(surface)



        camera.createCaptureSession(
            listOf(mImageReader.surface, surface),
            mSessionStateCallback,
            mHandler
        )
    }


    var time = 0L
    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        PathUtil.initVar(this)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        initWorker()

        val requestLocationPermission = registerForActivityResult(
            ActivityResultContracts.RequestMultiplePermissions()
        ) {
            for (k in it) {
                if (!k.value) {
                    Toast.makeText(this, "需要相机权限", Toast.LENGTH_LONG).show()
                    val intent = Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS)
                    val uri = Uri.fromParts("package", packageName, null)
                    intent.data = uri
                    startActivity(intent)
                    break;
                }
            }

            binding.texture.post {
                openCamera()
                resetWorker()
                time = 0L
                binding.btn.isClickable = false
                binding.btn.isFocusable = false
                binding.btn.text = "传输中"
                binding.pro.visibility = View.VISIBLE
                binding.hint.text = ""
            }
        }
        binding.btn.setOnClickListener {


            if (ContextCompat.checkSelfPermission(
                    this,
                    Manifest.permission.CAMERA
                ) != PackageManager.PERMISSION_GRANTED
            ) {
                requestLocationPermission.launch(
                    arrayOf(
                        Manifest.permission.CAMERA
                    )
                )
            } else {
                binding.texture.post {
                    openCamera()
                    resetWorker()
                    time = 0L
                    binding.btn.isClickable = false
                    binding.btn.isFocusable = false
                    binding.btn.text = "传输中"
                    binding.pro.visibility = View.VISIBLE
                    binding.hint.text = ""
                }
            }
        }






        Thread {
            while (true) {
                Thread.sleep(100)
                if (time == 0L) {
                    if (getProgress() != 0) {
                        time = System.nanoTime()
                    }
                }
                updateProgress(getProgress())
            }
        }.start()
    }


    fun decodeSuccess(b: ByteArray) {
        val file = File(PathUtil.getPathX("file2.pdf"))
        file.writeBytes(b)
        runOnUiThread {
            var text = "md5 check ok"
            val currentNano = System.nanoTime()
            val time = (currentNano - time) / 1000000000.0
            val df = DecimalFormat("#.00")
            var str = df.format(time)
            text += "\n耗时：$str s"
            binding.hint.text = text
            binding.btn.isClickable = true
            binding.btn.isFocusable = true
            binding.btn.text = "开始传输"
            closeCamera()
        }
    }

    fun updateProgress(progress: Int) {
        runOnUiThread {
            binding.pro.progress = progress
        }
    }

    private fun configureTransform(viewWidth: Int, viewHeight: Int) {
        val rotation = windowManager.defaultDisplay.rotation
        val matrix = Matrix()
        val viewRect = RectF(0F, 0F, viewWidth.toFloat(), viewHeight.toFloat())
        //RectF bufferRect = new RectF(0, 0, mPreviewSize.getHeight(), mPreviewSize.getWidth());
        val widthHeight = mPreviewSize.width.toFloat() / mPreviewSize.height
            .toFloat()
        val bottom = (viewWidth * widthHeight).toInt()
        val bufferRect = RectF(0F, 0F, viewWidth.toFloat(), bottom.toFloat())
        val centerX = viewRect.centerX()
        val centerY = viewRect.centerY()
        if (Surface.ROTATION_90 == rotation || Surface.ROTATION_270 == rotation) {
            bufferRect.offset(centerX - bufferRect.centerX(), centerY - bufferRect.centerY())
            matrix.setRectToRect(viewRect, bufferRect, Matrix.ScaleToFit.FILL)
            val scale = Math.max(
                viewHeight.toFloat() / mPreviewSize.height,
                viewWidth.toFloat() / mPreviewSize.width
            )
            matrix.postScale(scale, scale, centerX, centerY)
            matrix.postRotate((90 * (rotation - 2)).toFloat(), centerX, centerY)
        } else if (Surface.ROTATION_180 == rotation) {
            matrix.postRotate(180f, centerX, centerY)
        } else {
            bufferRect.offset(centerX - bufferRect.centerX(), centerY - bufferRect.centerY())
            matrix.setRectToRect(viewRect, bufferRect, Matrix.ScaleToFit.FILL)
        }
        binding.texture.setTransform(matrix)
    }

    override fun onPause() {
        super.onPause()
    }

    private fun closeCamera() {
        mCaptureSession.stopRepeating()
        mCaptureSession.close()
        mCameraDevice!!.close()
        mImageReader.close()
        stopBackgroundThread()
    }

    private fun stopBackgroundThread() {
        try {
            if (mHandlerThread != null) {
                mHandlerThread!!.quitSafely()
                mHandlerThread!!.join()
                mHandlerThread = null
            }
            mHandler.removeCallbacksAndMessages(null)
        } catch (e: InterruptedException) {
            e.printStackTrace()
        }
    }

}