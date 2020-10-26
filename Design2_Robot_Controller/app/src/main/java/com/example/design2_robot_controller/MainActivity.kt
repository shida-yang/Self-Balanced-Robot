package com.example.design2_robot_controller

import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothSocket
import android.graphics.Color
import android.os.Bundle
import android.os.Handler
import android.os.Message
import android.provider.SyncStateContract
import androidx.appcompat.app.AppCompatActivity
import io.github.controlwear.virtual.joystick.android.JoystickView.OnMoveListener
import kotlinx.android.synthetic.main.activity_main.*
import org.jetbrains.anko.toast
import java.util.*


class MainActivity : AppCompatActivity() {

    var bt_connected = false
    lateinit var bluetooth_adapter: BluetoothAdapter
    lateinit var bluetooth_socket: BluetoothSocket

    val buffer: ByteArray = ByteArray(32)

    val BT_MAC_ADDR: String = "98:D3:B1:FD:87:48"
    val MY_UUID: String = "00001101-0000-1000-8000-00805F9B34FB"

    lateinit var my_bt_connection_service: BluetoothConnectionService

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
//
//        val CONNECT_BT_BUTTON = findViewById<Button>(R.id.CONNECT_BT_BUTTON)
//        val STAND_BUTTON = findViewById<Button>(R.id.STAND_BUTTON)
//
//        val TILE_ANGLE_TEXT_VIEW = findViewById<TextView>(R.id.TILE_ANGLE_TEXT_VIEW)
//        val BT_STATUS_TEXT_VIEW = findViewById<TextView>(R.id.BT_STATUS_TEXT_VIEW)
//
//        val JS = findViewById<JoystickView>(R.id.JS)
//        val ANGLE_TEXT_VIEW = findViewById<TextView>(R.id.ANGLE_TEXT_VIEW)
//        val FORCE_TEXT_VIEW = findViewById<TextView>(R.id.FORCE_TEXT_VIEW)

        CONNECT_BT_BUTTON.setOnClickListener{
            if(bt_connected == false){
                bluetooth_adapter = BluetoothAdapter.getDefaultAdapter()
                val device: BluetoothDevice = bluetooth_adapter.getRemoteDevice(BT_MAC_ADDR)
                bluetooth_socket = device.createInsecureRfcommSocketToServiceRecord(
                    UUID.fromString(
                        MY_UUID
                    )
                )
                bluetooth_adapter.cancelDiscovery()
                toast("Please Wait, Connecting...")
                bluetooth_socket.connect()
                toast("Bluetooth Connected!")
                bt_connected = true
                BT_STATUS_TEXT_VIEW.setTextColor(Color.parseColor("#00FF00"))
                BT_STATUS_TEXT_VIEW.text = "BT Connected"
                my_bt_connection_service = BluetoothConnectionService(mHandler, bluetooth_socket)
                my_bt_connection_service.run()
            }
            else{
                BT_STATUS_TEXT_VIEW.setTextColor(Color.parseColor("#00FF00"))
                BT_STATUS_TEXT_VIEW.text = "BT Connected"
            }
        }



        JS.setOnMoveListener(OnMoveListener { angle, strength ->
            ANGLE_TEXT_VIEW.text = "Angle: $angle"
            FORCE_TEXT_VIEW.text = "Force: $strength"
            my_bt_connection_service.write("($angle,$strength)".toByteArray())
        })

    }

    private val mHandler = object : Handler(){
        override fun handleMessage(msg: Message) {
            if (msg.what == MESSAGE_READ){
                val buffer: ByteArray = msg.obj as ByteArray
                TILE_ANGLE_TEXT_VIEW.text = String(buffer, 0, msg.arg1)
            }

        }
    }

}