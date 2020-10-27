package com.example.design2_robot_controller

import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothProfile
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
import java.io.IOException
import java.lang.NumberFormatException
import java.util.*


class MainActivity : AppCompatActivity() {

    var bt_connected = false
    var bluetooth_adapter: BluetoothAdapter? = null
    var bluetooth_socket: BluetoothSocket? = null

    val buffer: ByteArray = ByteArray(1024)
    var buf_pos: Int = 0

    val BT_MAC_ADDR: String = "98:D3:B1:FD:87:48"
    val MY_UUID: String = "00001101-0000-1000-8000-00805F9B34FB"

    lateinit var my_bt_connection_service: BluetoothConnectionService

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        CONNECT_BT_BUTTON.setOnClickListener{
            if(!bt_connected){
                bluetooth_adapter = BluetoothAdapter.getDefaultAdapter()
                val device: BluetoothDevice = bluetooth_adapter!!.getRemoteDevice(BT_MAC_ADDR)
                if(bluetooth_adapter != null && device != null){
                    bluetooth_socket = device.createInsecureRfcommSocketToServiceRecord(
                        UUID.fromString(
                            MY_UUID
                        )
                    )
                    if(bluetooth_socket != null) {
                        try {
                            bluetooth_adapter!!.cancelDiscovery()
                            bluetooth_socket!!.connect()
                            toast("Bluetooth Connected!")
                            bt_connected = true
                            BT_STATUS_TEXT_VIEW.setTextColor(Color.parseColor("#00FF00"))
                            BT_STATUS_TEXT_VIEW.text = "BT Connected"
                            my_bt_connection_service =
                                BluetoothConnectionService(mHandler, bluetooth_socket!!)
                            my_bt_connection_service.run()
                        }catch(e: IOException){
                            toast("Cannot connect to HC-05")
                            e.printStackTrace()
                        }
                    }
                }
            }
            else{
                BT_STATUS_TEXT_VIEW.setTextColor(Color.parseColor("#00FF00"))
                BT_STATUS_TEXT_VIEW.text = "BT Connected"
            }
        }



        JS.setOnMoveListener(OnMoveListener { angle, strength ->
            ANGLE_TEXT_VIEW.text = "Angle: $angle"
            FORCE_TEXT_VIEW.text = "Force: $strength"
            if(bt_connected) {
                my_bt_connection_service.write("($angle,$strength)".toByteArray())
            }
        })

    }

    private val mHandler = object : Handler(){
        override fun handleMessage(msg: Message) {
            if (msg.what == MESSAGE_READ){
                val temp_buffer: ByteArray = msg.obj as ByteArray
                val len = msg.arg1
                for (i in 0 until len){
                    buffer[buf_pos] = temp_buffer[i];
                    buf_pos++
                    if(temp_buffer[i].compareTo(0) == 0) {
                        val temp_string = String(buffer, 0, buf_pos)
                        TILE_ANGLE_TEXT_VIEW.text = temp_string
                        buf_pos = 0
                    }
                }
//                val temp_string = String(buffer, 0, msg.arg1)
//                try {
//                    val temp_float = temp_string.toFloat()
//                    TILE_ANGLE_TEXT_VIEW.text = temp_string
//                }catch(e: NumberFormatException){
//                    e.printStackTrace()
//                }
            }
            else if(msg.what == MESSAGE_TOAST){
                bt_connected = false
                BT_STATUS_TEXT_VIEW.setTextColor(Color.parseColor("#FF0000"))
                BT_STATUS_TEXT_VIEW.text = "BT Not Connected"
                bluetooth_socket!!.close()
                my_bt_connection_service.cancel()
            }
        }
    }

}