#include "esp_camera.h"
#define CAMERA_MODEL_ESP32S3_EYE
#include "camera_pins.h"
#include "sd_read_write.h"
#include <Adafruit_NeoPixel.h>

#define BUTTON_PIN  0
#define LED_PIN     48
#define NUMPIXELS   1

Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

int cameraSetup(void); 

void setup() {
  Serial.begin(115200);
  delay(2000); // Give the USB Serial time to connect
  Serial.println("\n--- ESP32-S3 Starting Up ---");
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Initialize NeoPixel
  pixels.begin(); 
  pixels.setBrightness(50); 
  pixels.clear(); 
  pixels.show();

  // --- 1. SD CARD SETUP ---
  Serial.println("Initializing SD Card...");
  
  // Call the Freenove init function (returns void)
  sdmmcInit(); 
  
  // Check if the SD card is actually mounted
  if (SD_MMC.cardType() == CARD_NONE) {
    Serial.println("ERROR: SD Card Mount Failed!");
    Serial.println("Check: Is the SD card inserted? Is it formatted as FAT32?");
    pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // Red for SD error
    pixels.show();
    return; // Stop here if no SD card
  }
  
  Serial.println("SD Card Mounted Successfully.");
  
  createDir(SD_MMC, "/camera");
  Serial.println("Listing files in /camera:");
  listDir(SD_MMC, "/camera", 0);

  // --- 2. CAMERA SETUP ---
  Serial.println("Initializing Camera...");
  if(cameraSetup() == 1){
    pixels.setPixelColor(0, pixels.Color(0, 255, 0)); // Green for success
    pixels.show();
    Serial.println("--- System Ready! Press the BOOT button to take a photo ---");
  }
  else{
    pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // Red for camera error
    pixels.show();
    Serial.println("ERROR: Camera Initialization Failed!");
  }
}

void loop() {
  if(digitalRead(BUTTON_PIN) == LOW){
    delay(50); // Debounce
    if(digitalRead(BUTTON_PIN) == LOW){
      
      Serial.println("Button Pressed! Taking photo...");
      pixels.setPixelColor(0, pixels.Color(0, 0, 255)); // Blue while taking photo
      pixels.show();
      
      while(digitalRead(BUTTON_PIN) == LOW); // Wait for release
      
      camera_fb_t * fb = NULL;
      fb = esp_camera_fb_get();
      
      if (fb != NULL) {
        int photo_index = readFileNum(SD_MMC, "/camera");
        Serial.print("Photo Index: ");
        Serial.println(photo_index);

        if(photo_index != -1)
        {
          String path = "/camera/" + String(photo_index) + ".jpg";
          writejpg(SD_MMC, path.c_str(), fb->buf, fb->len);
          Serial.print("SUCCESS! Photo saved to: ");
          Serial.println(path);
        }
        esp_camera_fb_return(fb);
      }
      else {
        Serial.println("ERROR: Camera capture failed.");
      }
      
      // Return to Green when done
      pixels.setPixelColor(0, pixels.Color(0, 255, 0)); 
      pixels.show();
    }
  }
}

int cameraSetup(void) {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG; 
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  if(psramFound()){
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return 0;
  }

  sensor_t * s = esp_camera_sensor_get();
  s->set_vflip(s, 1); 
  s->set_brightness(s, 1); 
  s->set_saturation(s, 0); 

  Serial.println("Camera configuration complete!");
  return 1;
}
