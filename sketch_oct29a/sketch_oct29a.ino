#include "WiFi.h"
#include "esp_camera.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "Base64.h"
#include "camera_pins.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

//coloca o nome e a senha do wi-fi aqui
const char *ssid = "";
const char *password = "";

//coloca a URL e a chave API do seu firebase aqui.
const String FIREBASE_URL = "";
const String API_KEY = "";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800, 60000);

void setup() {
  Serial.begin(115200);
  connectWiFi();
  cameraInit();
  timeClient.begin();
}

//chamada da função que envia imagem pro Firebase com o delay setado (30 segundos só que em milisegundos)
void loop() {
  sendImageToFirebase();
  delay(30000);
}

//Função de conexão à internet
void connectWiFi() {
  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado ao Wi-Fi!");
}

//setagem de pinos do esp32 cam usando AI Thinker(extraí do exemplo pra ser mais fácil)
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
  config.frame_size = FRAMESIZE_VGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 10;
  config.fb_count = 2;

  //teste de câmera
  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Falha ao inicializar a câmera.");
    while (true);
  }
  Serial.println("Câmera inicializada com sucesso!");
}

//conversão da imagem para base64
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
  Serial.println ("Imagem em Base64: " + imageBase64);

  StaticJsonDocument<150000> doc;
  doc["timestamp"] = millis();
  doc["imagembase64"] = imageBase64;
  doc["datahora"] = getFormattedTime();
  
  String json;
  serializeJson(doc, json);

  if (sendDataToFirebase(json)) {
    Serial.println("Imagem enviada com sucesso!");
  } else {
    Serial.println("Erro ao enviar imagem.");
  }
}

String getFormattedTime() {
  timeClient.update();
  return timeClient.getFormattedTime();
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