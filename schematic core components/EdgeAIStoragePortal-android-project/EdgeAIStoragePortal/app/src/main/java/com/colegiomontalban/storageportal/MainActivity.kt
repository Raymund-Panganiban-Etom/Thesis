package com.colegiomontalban.storageportal

import android.Manifest
import android.annotation.SuppressLint
import android.app.Activity
import android.content.pm.PackageManager
import android.graphics.Color
import android.os.Build
import android.os.Bundle
import android.webkit.WebSettings
import android.webkit.WebView
import android.webkit.WebViewClient

class MainActivity : Activity() {

    private lateinit var web: WebView
    private lateinit var ble: BleBridge
    private var pendingAfterPermission: (() -> Unit)? = null

    @SuppressLint("SetJavaScriptEnabled")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        web = WebView(this)
        web.setBackgroundColor(Color.parseColor("#123022"))
        setContentView(web)

        web.settings.apply {
            javaScriptEnabled = true
            domStorageEnabled = true          // localStorage (theme preference)
            allowFileAccess = true
            cacheMode = WebSettings.LOAD_DEFAULT
        }
        web.webViewClient = WebViewClient()

        ble = BleBridge(this, web)
        // Must be registered before loadUrl so window.AndroidBle exists when app.js runs.
        web.addJavascriptInterface(ble, "AndroidBle")

        web.loadUrl("file:///android_asset/index.html")
    }

    /** Runs [block] once the Bluetooth runtime permissions are granted. */
    fun withBlePermissions(block: () -> Unit) {
        val needed = if (Build.VERSION.SDK_INT >= 31)
            arrayOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT)
        else
            arrayOf(Manifest.permission.ACCESS_FINE_LOCATION)

        val missing = needed.filter { checkSelfPermission(it) != PackageManager.PERMISSION_GRANTED }
        if (missing.isEmpty()) {
            block()
        } else {
            pendingAfterPermission = block
            requestPermissions(missing.toTypedArray(), 42)
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode != 42) return
        val block = pendingAfterPermission
        pendingAfterPermission = null
        if (grantResults.isNotEmpty() && grantResults.all { it == PackageManager.PERMISSION_GRANTED }) block?.invoke()
        else ble.permissionDenied()
    }

    @Deprecated("Deprecated in Java")
    override fun onBackPressed() {
        // From the live-readings screen, go back to crop selection instead of closing the app.
        web.evaluateJavascript(
            "(function(){var f=document.getElementById('first'),b=document.getElementById('back');" +
            "if(f&&f.hidden&&b){b.click();return 'handled';}return 'exit';})()"
        ) { result -> if (result != "\"handled\"") finish() }
    }

    override fun onDestroy() {
        ble.destroy()
        web.destroy()
        super.onDestroy()
    }
}
