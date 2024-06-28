package com.example.testandroidcpp

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.widget.TextView
import com.example.testandroidcpp.databinding.ActivityMainBinding
import kotlin.concurrent.thread

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // Example of a call to a native method
        binding.sampleText.text = stringFromJNI()
        Log.i("LocalSocketServer", "onCreate: before server kotlin")
    thread {
        testLocalSocketServer()
    }
    }

    /**
     * A native method that is implemented by the 'testandroidcpp' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String
    external fun testLocalSocketServer()

    companion object {
        // Used to load the 'testandroidcpp' library on application startup.
        init {
            System.loadLibrary("testandroidcpp")
        }
    }
}