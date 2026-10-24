package com.colegiomontalban.storageportal

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothProfile
import android.bluetooth.le.BluetoothLeScanner
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.os.ParcelUuid
import android.webkit.JavascriptInterface
import android.webkit.WebView
import org.json.JSONObject
import java.util.UUID

/**
 * Native BLE bridge for the web UI.
 *
 * JS -> native : window.AndroidBle.connect() / window.AndroidBle.disconnect()
 * native -> JS : window.NativeBLE.onState / onConnected / onDisconnected / onError / onData
 *
 * Talks to the ESP32 over the Nordic UART Service; the ESP32 notifies text lines on the TX characteristic.
 */
@SuppressLint("MissingPermission")
class BleBridge(private val activity: MainActivity, private val web: WebView) {

    companion object {
        val SERVICE: UUID = UUID.fromString("6e400001-b5a8-f393-e0a9-e50e24dcca9e")
        val TX_CHAR: UUID = UUID.fromString("6e400003-b5a8-f393-e0a9-e50e24dcca9e")
        val CCCD: UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")
        const val DEVICE_NAME = "ESP32-Storage"
        const val SCAN_TIMEOUT_MS = 12_000L
    }

    private val main = Handler(Looper.getMainLooper())
    private var gatt: BluetoothGatt? = null
    private var scanner: BluetoothLeScanner? = null
    private var scanning = false
    private var linkUp = false
    private var deviceName = DEVICE_NAME

    private val scanTimeout = Runnable {
        if (scanning) {
            stopScan()
            toJs("onError", "$DEVICE_NAME not found. Make sure it is powered on and nearby.")
        }
    }

    // ---------- native -> JS ----------
    private fun toJs(fn: String, arg: String? = null) {
        val call = if (arg == null) "$fn()" else "$fn(${JSONObject.quote(arg)})"
        web.post { web.evaluateJavascript("window.NativeBLE && window.NativeBLE.$call", null) }
    }

    fun permissionDenied() = toJs("onError", "Bluetooth permission denied. Allow it in Settings > Apps.")

    // ---------- JS -> native ----------
    @JavascriptInterface
    fun connect() {
        main.post { activity.withBlePermissions { startScan() } }
    }

    @JavascriptInterface
    fun disconnect() {
        main.post {
            stopScan()
            gatt?.disconnect()
        }
    }

    // ---------- scanning ----------
    private fun startScan() {
        val adapter: BluetoothAdapter? =
            (activity.getSystemService(android.content.Context.BLUETOOTH_SERVICE) as BluetoothManager).adapter
        if (adapter == null || !adapter.isEnabled) {
            toJs("onError", "Bluetooth is off. Turn it on and tap Connect again.")
            return
        }
        if (scanning) return
        closeGatt()

        scanner = adapter.bluetoothLeScanner
        if (scanner == null) {
            toJs("onError", "Bluetooth LE scanner unavailable.")
            return
        }
        val filters = listOf(
            ScanFilter.Builder().setServiceUuid(ParcelUuid(SERVICE)).build(),
            ScanFilter.Builder().setDeviceName(DEVICE_NAME).build()
        )
        val settings = ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build()
        scanning = true
        toJs("onState", "Scanning for $DEVICE_NAME…")
        scanner?.startScan(filters, settings, scanCallback)
        main.postDelayed(scanTimeout, SCAN_TIMEOUT_MS)
    }

    private fun stopScan() {
        main.removeCallbacks(scanTimeout)
        if (scanning) {
            try { scanner?.stopScan(scanCallback) } catch (_: Exception) {}
        }
        scanning = false
    }

    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            if (!scanning) return
            stopScan()
            val dev: BluetoothDevice = result.device
            deviceName = dev.name ?: DEVICE_NAME
            toJs("onState", "Connecting to $deviceName…")
            gatt = dev.connectGatt(activity, false, gattCallback, BluetoothDevice.TRANSPORT_LE)
        }

        override fun onScanFailed(errorCode: Int) {
            scanning = false
            main.removeCallbacks(scanTimeout)
            toJs("onError", "Scan failed (code $errorCode).")
        }
    }

    // ---------- GATT ----------
    private fun closeGatt() {
        try { gatt?.close() } catch (_: Exception) {}
        gatt = null
        linkUp = false
    }

    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(g: BluetoothGatt, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED && status == BluetoothGatt.GATT_SUCCESS) {
                // Bigger MTU so each text line fits in few notifications; fall back if it can't be requested.
                if (!g.requestMtu(185)) g.discoverServices()
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                val wasUp = linkUp
                main.post {
                    closeGatt()
                    if (wasUp) toJs("onDisconnected")
                    else if (status != BluetoothGatt.GATT_SUCCESS) toJs("onError", "Connection failed (status $status).")
                }
            }
        }

        override fun onMtuChanged(g: BluetoothGatt, mtu: Int, status: Int) {
            g.discoverServices()
        }

        override fun onServicesDiscovered(g: BluetoothGatt, status: Int) {
            val ch = g.getService(SERVICE)?.getCharacteristic(TX_CHAR)
            val cccd = ch?.getDescriptor(CCCD)
            if (status != BluetoothGatt.GATT_SUCCESS || ch == null || cccd == null) {
                toJs("onError", "ESP32 UART service not found on this device.")
                g.disconnect()
                return
            }
            g.setCharacteristicNotification(ch, true)
            if (Build.VERSION.SDK_INT >= 33) {
                g.writeDescriptor(cccd, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)
            } else {
                @Suppress("DEPRECATION")
                cccd.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                @Suppress("DEPRECATION")
                g.writeDescriptor(cccd)
            }
        }

        override fun onDescriptorWrite(g: BluetoothGatt, d: BluetoothGattDescriptor, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                linkUp = true
                toJs("onConnected", deviceName)
            } else {
                toJs("onError", "Could not enable notifications (status $status).")
                g.disconnect()
            }
        }

        // On Android 13+ the framework forwards the new overload to this one, so it works on every version.
        @Deprecated("Deprecated in API 33; still invoked on all versions")
        override fun onCharacteristicChanged(g: BluetoothGatt, c: BluetoothGattCharacteristic) {
            val bytes = c.value ?: return
            toJs("onData", String(bytes, Charsets.UTF_8))
        }
    }

    fun destroy() {
        stopScan()
        try { gatt?.disconnect() } catch (_: Exception) {}
        closeGatt()
    }
}
