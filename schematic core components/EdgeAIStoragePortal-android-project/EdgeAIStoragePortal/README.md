# Edge AI Storage Portal – Android app

Your web app (`app/src/main/assets/`) wrapped in a WebView, with a native Bluetooth LE bridge
(`BleBridge.kt`) that implements the `window.AndroidBle` / `window.NativeBLE` hooks your `app.js` already uses.
Web Bluetooth does not exist inside Android WebViews, so the bridge is what makes "Connect" work.

## Build the APK – option A: GitHub (no installs)
1. Create a new GitHub repository and upload everything in this folder (keep the `.github` folder).
2. Open the **Actions** tab > **Build APK** > **Run workflow** (it also runs on every push).
3. When it finishes, open the run and download the **EdgeAIStoragePortal-apk** artifact. Unzip it to get `app-debug.apk`.

## Build the APK – option B: Android Studio
1. Install Android Studio, then **File > Open** and pick this folder. Let Gradle sync.
   (If it complains about a missing Gradle wrapper, run `gradle wrapper --gradle-version 8.7` once, or set a local Gradle 8.7.)
2. **Build > Build Bundle(s) / APK(s) > Build APK(s)**.
3. The file is at `app/build/outputs/apk/debug/app-debug.apk`.

## Install
Copy the APK to the phone, open it and allow "Install unknown apps". On first Connect, allow the Nearby devices
(Bluetooth) permission. On Android 11 or older, Location permission and the Location switch must be on for BLE scanning.

## Crop photos
Put your photos in `app/src/main/assets/images/` named `calamansi.jpg`, `corn.jpg`, `pomelo.jpg`,
`string-beans.jpg`, `banana.jpg`. Without them the app shows the gradient fallbacks.

## ESP32 expectations (unchanged from your app)
Device name `ESP32-Storage`, Nordic UART service `6e400001-...`, notifications on TX `6e400003-...`, one text line per
reading ending in `\n`: `T1:..,H1:..,E1:..,T2:..,H2:..,E2:..`
