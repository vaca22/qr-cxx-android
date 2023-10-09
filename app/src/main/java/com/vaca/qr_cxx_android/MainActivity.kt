package com.vaca.qr_cxx_android

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.TextView
import com.vaca.qr_cxx_android.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)


        binding.sampleText.text = stringFromJNI()


        //read assets file
        val inputStream = assets.open("dd22.jpg")
        val size = inputStream.available()
        val buffer = ByteArray(size)
        inputStream.read(buffer)
        inputStream.close()
        qr(buffer)
    }


    external fun stringFromJNI(): String
    external fun qr(b:ByteArray)

    companion object {
        // Used to load the 'qr_cxx_android' library on application startup.
        init {
            System.loadLibrary("qr_cxx_android")
        }
    }
}