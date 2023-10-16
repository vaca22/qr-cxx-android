package com.vaca.qr_cxx_android

import android.Manifest
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.ImageFormat
import android.graphics.Rect
import android.graphics.YuvImage
import android.hardware.camera2.*
import android.media.Image
import android.media.ImageReader
import android.net.Uri
import android.os.Build.VERSION_CODES.P
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.os.Handler
import android.os.HandlerThread
import android.provider.Settings
import android.util.Size
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.content.ContextCompat
import com.vaca.qr_cxx_android.databinding.ActivityMainBinding
import java.io.ByteArrayOutputStream
import java.io.File

class MainActivity : AppCompatActivity() {

    private val mCameraId = "0"
    lateinit var mPreviewSize: Size
    private val PREVIEW_WIDTH = 1080
    private val PREVIEW_HEIGHT = 1920
    private var mCameraDevice: CameraDevice? = null
    lateinit var mHandler: Handler
    lateinit var mCaptureSession: CameraCaptureSession
    lateinit var mPreviewBuilder: CaptureRequest.Builder
    private var mHandlerThread: HandlerThread? = null
    lateinit var mImageReader: ImageReader



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
    private fun convertYUV_420_888toNV21(image: Image): ByteArray {
        val nv21: ByteArray
        val yBuffer = image.planes[0].buffer
        val vuBuffer = image.planes[2].buffer
        val ySize = yBuffer.remaining()
        val vuSize = vuBuffer.remaining()
        nv21 = ByteArray(ySize + vuSize)
        yBuffer[nv21, 0, ySize]
        vuBuffer[nv21, ySize, vuSize]
        return nv21
    }

    private fun convertNV21toJPEG(nv21: ByteArray, width: Int, height: Int): ByteArray {
        val out = ByteArrayOutputStream()
        val yuv = YuvImage(nv21, ImageFormat.NV21, width, height, null)
        yuv.compressToJpeg(Rect(0, 0, width, height), 60, out)
        return out.toByteArray()
    }
    private inner class ImageSaver(var reader: ImageReader) : Runnable {
        override fun run() {
            val image = reader.acquireLatestImage() ?: return
            val data = convertNV21toJPEG(
                convertYUV_420_888toNV21(image),
                image.width, image.height
            )
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


        camera.createCaptureSession(
            listOf(mImageReader.surface),
            mSessionStateCallback,
            mHandler
        )
    }







    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        PathUtil.initVar(this)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        initWorker()




        startBackgroundThread()

        val requestLocationPermission = registerForActivityResult(
            ActivityResultContracts.RequestMultiplePermissions()
        ) {
            for(k in it){
                if(!k.value){
                    Toast.makeText(this,"需要相机权限", Toast.LENGTH_LONG).show()
                    val intent = Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS)
                    val uri = Uri.fromParts("package", packageName, null)
                    intent.data = uri
                    startActivity(intent)
                    break;
                }
            }

            openCamera()

        }
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
        }else{
            openCamera()
        }



        //update progress every 1 s
        Thread{
            while(true){
                Thread.sleep(200)
                updateProgress(getProgress())
            }
        }.start()
    }



    external fun inputImage(b:ByteArray)
    external fun initWorker()
    external fun getProgress():Int

    companion object {
        init {
            System.loadLibrary("qr_cxx_android")
        }
    }


    fun decodeSuccess(b: ByteArray){
        val file= File(PathUtil.getPathX("file2.pdf"))
        file.writeBytes(b)
        runOnUiThread {
            binding.hint.text="md5 check success"
        }
    }

    fun updateProgress(progress:Int){
        runOnUiThread {
            binding.pro.progress=progress
        }
    }
}