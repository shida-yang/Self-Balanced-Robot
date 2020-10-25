package com.example.design2_robot_controller

import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.content.Intent
import android.os.Bundle
import android.widget.Button
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import io.github.controlwear.virtual.joystick.android.JoystickView
import io.github.controlwear.virtual.joystick.android.JoystickView.OnMoveListener

class MainActivity : AppCompatActivity() {

    lateinit var bluetooth_adapter: BluetoothAdapter
    lateinit var paired_device: BluetoothDevice

    var bt_connected = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        val CONNECT_BT_BUTTON = findViewById<Button>(R.id.CONNECT_BT_BUTTON)
        val STAND_BUTTON = findViewById<Button>(R.id.STAND_BUTTON)

        val TILE_ANGLE_TEXT_VIEW = findViewById<TextView>(R.id.TILE_ANGLE_TEXT_VIEW)
        val BT_STATUS_TEXT_VIEW = findViewById<TextView>(R.id.BT_STATUS_TEXT_VIEW)

        val JS = findViewById<JoystickView>(R.id.JS)
        val ANGLE_TEXT_VIEW = findViewById<TextView>(R.id.ANGLE_TEXT_VIEW)
        val FORCE_TEXT_VIEW = findViewById<TextView>(R.id.FORCE_TEXT_VIEW)

        CONNECT_BT_BUTTON.setOnClickListener{
            startActivity(Intent(this@MainActivity, ConnectBTActivity::class.java))
//            if (bt_connected) {
//                BT_STATUS_TEXT_VIEW.setTextColor(Color.parseColor("#FF0000"))
//                BT_STATUS_TEXT_VIEW.text = "BT Not Connected"
//                bt_connected = false
//            }
//            else{
//                BT_STATUS_TEXT_VIEW.setTextColor(Color.parseColor("#00FF00"))
//                BT_STATUS_TEXT_VIEW.text = "BT Connected"
//                bt_connected = true
//            }
        }

        JS.setOnMoveListener(OnMoveListener { angle, strength ->
            ANGLE_TEXT_VIEW.text = "Angle: $angle"
            FORCE_TEXT_VIEW.text = "Force: $strength"
        })
    }
}