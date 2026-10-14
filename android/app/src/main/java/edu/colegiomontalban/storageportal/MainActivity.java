package edu.colegiomontalban.storageportal;

import android.Manifest;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothStatusCodes;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelUuid;
import android.webkit.JavascriptInterface;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import org.json.JSONObject;

import java.util.Collections;
import java.util.UUID;

public class MainActivity extends Activity {
    private static final int PERMISSION_REQUEST = 41;
    private static final UUID SERVICE_UUID =
            UUID.fromString("6e400001-b5a8-f393-e0a9-e50e24dcca9e");
    private static final UUID TX_CHAR_UUID =
            UUID.fromString("6e400003-b5a8-f393-e0a9-e50e24dcca9e");
    private static final UUID CCCD_UUID =
            UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");

    private final Handler handler = new Handler(Looper.getMainLooper());
    private WebView webView;
    private BluetoothAdapter bluetoothAdapter;
    private BluetoothLeScanner scanner;
    private BluetoothGatt gatt;
    private BluetoothGattCharacteristic txCharacteristic;
    private boolean scanning;
    private boolean permissionRequestPending;

    private final Runnable scanTimeout = () -> {
        if (scanning) {
            stopScan();
            sendError("ESP32-Storage not found — make sure it is powered on and try again");
        }
    };

    private final ScanCallback scanCallback = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            stopScan();
            runOnUiThread(() -> connectGatt(result.getDevice()));
        }

        @Override
        public void onScanFailed(int errorCode) {
            stopScan();
            sendError("Bluetooth scan failed — check Bluetooth permissions and try again");
        }
    };

    private final BluetoothGattCallback gattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt connectedGatt, int status, int newState) {
            if (status != BluetoothGatt.GATT_SUCCESS) {
                closeGatt();
                sendError("Could not connect to ESP32-Storage — power-cycle it and try again");
                return;
            }
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                connectedGatt.discoverServices();
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                closeGatt();
                sendDisconnected();
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt discoveredGatt, int status) {
            if (status != BluetoothGatt.GATT_SUCCESS) {
                closeGatt();
                sendError("Could not read ESP32 services — restart the sensor and try again");
                return;
            }
            android.bluetooth.BluetoothGattService service =
                    discoveredGatt.getService(SERVICE_UUID);
            txCharacteristic = service == null ? null : service.getCharacteristic(TX_CHAR_UUID);
            if (txCharacteristic == null) {
                closeGatt();
                sendError("ESP32 service or characteristic is missing — check its firmware");
                return;
            }
            if (!discoveredGatt.setCharacteristicNotification(txCharacteristic, true)) {
                closeGatt();
                sendError("Could not enable ESP32 sensor updates");
                return;
            }
            BluetoothGattDescriptor descriptor = txCharacteristic.getDescriptor(CCCD_UUID);
            if (descriptor == null) {
                closeGatt();
                sendError("ESP32 notification descriptor is missing — check its firmware");
                return;
            }
            boolean started;
            if (Build.VERSION.SDK_INT >= 33) {
                started = discoveredGatt.writeDescriptor(
                        descriptor, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)
                        == BluetoothStatusCodes.SUCCESS;
            } else {
                descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
                started = discoveredGatt.writeDescriptor(descriptor);
            }
            if (!started) {
                closeGatt();
                sendError("Could not subscribe to ESP32 sensor updates");
            }
        }

        @Override
        public void onDescriptorWrite(
                BluetoothGatt writtenGatt, BluetoothGattDescriptor descriptor, int status) {
            if (CCCD_UUID.equals(descriptor.getUuid())) {
                if (status == BluetoothGatt.GATT_SUCCESS) {
                    String name = writtenGatt.getDevice().getName();
                    sendConnected(name == null ? "ESP32-Storage" : name);
                } else {
                    closeGatt();
                    sendError("Could not subscribe to ESP32 sensor updates");
                }
            }
        }

        @Override
        public void onCharacteristicChanged(
                BluetoothGatt changedGatt, BluetoothGattCharacteristic characteristic) {
            if (TX_CHAR_UUID.equals(characteristic.getUuid())) {
                sendData(characteristic.getValue());
            }
        }

        @Override
        public void onCharacteristicChanged(
                BluetoothGatt changedGatt,
                BluetoothGattCharacteristic characteristic,
                byte[] value) {
            if (TX_CHAR_UUID.equals(characteristic.getUuid())) {
                sendData(value);
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        BluetoothManager manager =
                (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        bluetoothAdapter = manager == null ? null : manager.getAdapter();

        webView = new WebView(this);
        webView.getSettings().setJavaScriptEnabled(true);
        webView.getSettings().setDomStorageEnabled(true);
        webView.setWebViewClient(new WebViewClient());
        webView.addJavascriptInterface(new AndroidBleBridge(), "AndroidBle");
        setContentView(webView);
        webView.loadUrl("file:///android_asset/www/index.html");
    }

    public class AndroidBleBridge {
        @JavascriptInterface
        public void connect() {
            runOnUiThread(() -> {
                if (!hasBluetoothPermissions()) {
                    permissionRequestPending = true;
                    requestPermissions(requiredPermissions(), PERMISSION_REQUEST);
                    return;
                }
                startScan();
            });
        }

        @JavascriptInterface
        public void disconnect() {
            runOnUiThread(() -> {
                permissionRequestPending = false;
                stopScan();
                if (gatt != null) {
                    gatt.disconnect();
                } else {
                    sendDisconnected();
                }
            });
        }
    }

    private String[] requiredPermissions() {
        if (Build.VERSION.SDK_INT >= 31) {
            return new String[] {
                    Manifest.permission.BLUETOOTH_SCAN,
                    Manifest.permission.BLUETOOTH_CONNECT
            };
        }
        if (Build.VERSION.SDK_INT >= 23) {
            return new String[] { Manifest.permission.ACCESS_FINE_LOCATION };
        }
        return new String[0];
    }

    private boolean hasBluetoothPermissions() {
        if (Build.VERSION.SDK_INT < 23) return true;
        for (String permission : requiredPermissions()) {
            if (checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED) return false;
        }
        return true;
    }

    @Override
    public void onRequestPermissionsResult(
            int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode != PERMISSION_REQUEST || !permissionRequestPending) return;

        permissionRequestPending = false;
        for (int result : grantResults) {
            if (result != PackageManager.PERMISSION_GRANTED) {
                sendError("Bluetooth permission is required to connect to ESP32-Storage");
                return;
            }
        }
        startScan();
    }

    private void startScan() {
        if (bluetoothAdapter == null || !bluetoothAdapter.isEnabled()) {
            sendError("Turn on Bluetooth, then tap Connect to try again");
            return;
        }
        scanner = bluetoothAdapter.getBluetoothLeScanner();
        if (scanner == null) {
            sendError("Bluetooth scanning is unavailable on this device");
            return;
        }
        try {
            ScanFilter filter = new ScanFilter.Builder()
                    .setServiceUuid(new ParcelUuid(SERVICE_UUID))
                    .build();
            ScanSettings settings = new ScanSettings.Builder()
                    .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                    .build();
            scanning = true;
            sendState("Scanning for ESP32-Storage…");
            scanner.startScan(Collections.singletonList(filter), settings, scanCallback);
            handler.postDelayed(scanTimeout, 15_000);
        } catch (SecurityException exception) {
            stopScan();
            sendError("Bluetooth permission is required to scan for ESP32-Storage");
        }
    }

    private void connectGatt(BluetoothDevice device) {
        if (device == null) {
            sendError("ESP32-Storage not found — make sure it is powered on and try again");
            return;
        }
        try {
            String name = device.getName();
            sendState("Connecting to " + (name == null ? "ESP32-Storage" : name) + "…");
            gatt = device.connectGatt(this, false, gattCallback);
            if (gatt == null) {
                sendError("Could not start the ESP32 Bluetooth connection");
            }
        } catch (SecurityException exception) {
            sendError("Bluetooth permission is required to connect to ESP32-Storage");
        }
    }

    private void stopScan() {
        handler.removeCallbacks(scanTimeout);
        if (!scanning) return;
        scanning = false;
        if (scanner != null && hasBluetoothPermissions()) {
            try {
                scanner.stopScan(scanCallback);
            } catch (SecurityException ignored) {
                // The scan is already stopped if Android has revoked permission.
            }
        }
    }

    private void closeGatt() {
        BluetoothGatt oldGatt = gatt;
        gatt = null;
        txCharacteristic = null;
        if (oldGatt != null) {
            try {
                oldGatt.close();
            } catch (SecurityException ignored) {
                // Android may revoke Bluetooth permission while the connection closes.
            }
        }
    }

    private void sendState(String message) {
        evaluate("window.NativeBLE && window.NativeBLE.onState("
                + JSONObject.quote(message) + ")");
    }

    private void sendConnected(String name) {
        evaluate("window.NativeBLE && window.NativeBLE.onConnected("
                + JSONObject.quote(name) + ")");
    }

    private void sendDisconnected() {
        evaluate("window.NativeBLE && window.NativeBLE.onDisconnected()");
    }

    private void sendError(String message) {
        evaluate("window.NativeBLE && window.NativeBLE.onError("
                + JSONObject.quote(message) + ")");
    }

    private void sendData(byte[] value) {
        if (value == null) return;
        String text = new String(value, java.nio.charset.StandardCharsets.UTF_8);
        evaluate("window.NativeBLE && window.NativeBLE.onData("
                + JSONObject.quote(text) + ")");
    }

    private void evaluate(String javascript) {
        runOnUiThread(() -> {
            if (webView != null) webView.evaluateJavascript(javascript, null);
        });
    }

    @Override
    protected void onDestroy() {
        stopScan();
        closeGatt();
        if (webView != null) {
            webView.removeJavascriptInterface("AndroidBle");
            webView.destroy();
            webView = null;
        }
        super.onDestroy();
    }
}
