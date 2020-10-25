package com.example.design2_robot_controller

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle

class ConnectBTActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_connect_bt)

        val actionBar = supportActionBar
        actionBar!!.title = "Connecting Bluetooth"
        actionBar.setDisplayHomeAsUpEnabled(true)
    }

    override fun onSupportNavigateUp(): Boolean {
        onBackPressed()
        return true
    }
}