#include "WiFi.h"
#include "esp_camera.h"
#include <ArduinoJson.h>
#include <HTTPClient.h> 


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
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sccb_sda = 26;
  config.pin_sccb_scl = 27;
  config.pin_reset = -1; 
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Falha ao inicializar a câmera.");
    while (true); 
  }
  Serial.println("Câmera inicializada com sucesso!");
}


void sendImageToFirebase() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Falha ao capturar imagem.");
    return;
  }


  StaticJsonDocument<50000> doc;
  doc["timestamp"] = millis();
  doc["image"] = Image;

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
