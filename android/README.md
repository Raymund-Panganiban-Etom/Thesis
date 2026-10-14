# Android app

The Android app packages the web portal as an offline-capable WebView and uses
native Bluetooth LE for connecting to the ESP32-Storage sensor.

## Build

Open the `android` folder in Android Studio, allow Gradle to sync, then run the
`app` debug build. The command-line equivalent is:

```powershell
.\gradlew.bat assembleDebug
```

The debug APK is written to
`app\build\outputs\apk\debug\app-debug.apk`. To install it with Android Debug
Bridge, connect an Android device with USB debugging enabled and run:

```powershell
adb install -r app\build\outputs\apk\debug\app-debug.apk
```

The app asks for Bluetooth permissions when connecting. On Android 11 and
earlier, Android also asks for location permission because those releases
require it for BLE scanning; the app does not use location data.

## Install the provided APK

Copy the APK to an Android device, open it, and approve installation from that
source if Android prompts. Launch **Storage Portal**, choose a crop, then tap
**Connect** and allow nearby-device access to connect to the ESP32.
