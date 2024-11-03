#include "WiFi.h"
#include "esp_camera.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "Base64.h"  
#include "camera_pins.h"


const char* ssid = "";
const char* password = "";

const String FIREBASE_URL = "https://firebasestorage.googleapis.com/v0/b/esp32-cam-31201.appspot.com/o/";
const String API_KEY = "AIzaSyA__VKYgNW6Wbl0EALZ3pRfMr2Dva2FdrQ"; 

void setup() {
  Serial.begin(115200);
  connectWiFi();
  cameraInit();
}

void loop() {
  sendImageToFirebase();
  delay(30000); 
}

void connectWiFi() {
  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado ao Wi-Fi!");
}

void cameraInit() {
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
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG; 
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }
  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Falha ao inicializar a câmera.");
    while (true); 
  }
  Serial.println("Câmera inicializada com sucesso!");
}

String convertImageToBase64(camera_fb_t *fb) {
  return base64::encode(fb->buf, fb->len);
}

void sendImageToFirebase() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Falha ao capturar imagem.");
    return;
  }

  String imageBase64 = convertImageToBase64(fb);
  esp_camera_fb_return(fb); 

  StaticJsonDocument<50000> doc;
  doc["timestamp"] = millis();
  doc["image"] = imageBase64;

  String json;
  serializeJson(doc, json);

  if (sendDataToFirebase(json)) {
    Serial.println("Imagem enviada com sucesso!");
  } else {
    Serial.println("Erro ao enviar imagem.");
  }
}

bool sendDataToFirebase(const String &json) {
  HTTPClient http;
  http.begin(FIREBASE_URL + "?auth=" + API_KEY);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(json);
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Resposta do Firebase: " + response);
    http.end();
    return true;
  } else {
    Serial.println("Erro HTTP: " + String(httpResponseCode));
    http.end();
    return false;
  }
}
