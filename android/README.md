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
========================================================================================
so basically, just download the Edge-AI-Storage-Portal-v1.0.0.apk
then this android folder is a editable (open source to improve this application)

here's the convo for question:

give me step by step how to install that application in my phone

## Install the app using a USB cable

1. **Download the APK** to your computer: `Edge AI Storage Portal v1.0.0`.

2. **Connect your Android phone** to the computer with a USB cable. Unlock the phone, tap the USB notification, and select **File transfer** (sometimes called **Transferring files**).

3. On your computer, open your phone’s storage in File Explorer. Copy `Edge-AI-Storage-Portal-v1.0.0.apk` into the phone’s **Download** folder.

4. **Eject or disconnect the phone**, then open its **Files** or **My Files** app. Find the APK in **Downloads** and tap it.

5. If Android asks you to allow installation, tap **Settings** on the warning and enable **Allow from this source** for the app you’re using to open the APK (usually Files or My Files). Go back and tap the APK again, then tap **Install**.

6. Once installation finishes, tap **Open**. You can also find **Storage Portal** in your app list.

7. To connect the sensor, turn on Bluetooth, tap **Get started**, choose a crop, and tap **Connect**. Allow the requested Bluetooth or nearby-device permission. On older Android versions, it may also request location permission for Bluetooth scanning; the app does not use your location.

You can turn off **Allow from this source** again after installation.


so apk version all needed? then what about the android folder? is it a folder where it can edit for future update?


Yes—the **APK contains what your phone needs to install and run the app**. You don’t need to copy the `android` folder to your phone.

The `android folder` is the **editable Android project** for future changes. It contains the native Android wrapper, Bluetooth integration, app settings, and build configuration. The website files—such as `app.js`, `index.html`, and `style.css`—are in the main project folder. The build packages those files into the APK.

For a future update, edit the relevant project files, rebuild from the `android` folder using the command in `README.md`, then install the newly built APK on your phone. **The APK currently provided is a debug build**, suitable for testing and personal installation; publishing on Google Play requires a separately signed release build.



then this folder will be open in android studio and after improvement or revise, the android studio can make the apk file?

Yes. Open the **`android` folder** in Android Studio as the project. Android Studio can build an APK after you revise the app.

- To update the app’s screens, styling, or web behavior, edit the files in the main project folder, such as `app.js`, `index.html`, and `style.css`. The Android build includes those files automatically.
- To update Android-specific behavior or settings, edit the files inside `android`.
- In Android Studio, choose **Build → Build Bundle(s) / APK(s) → Build APK(s)**. The APK will be created under `android\app\build\outputs\apk\debug\`.

For an updated APK to install as an **update over the existing app**, keep the same signing key and increase the app’s `versionCode` in `android\app\build.gradle` for each release. The current APK is a debug build for testing/personal installation; distributing through Google Play requires a signed release build.

just download the ino file for ESP32+DHT11 FOR bt connection



