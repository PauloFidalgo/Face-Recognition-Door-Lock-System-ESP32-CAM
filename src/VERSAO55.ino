#include "esp_camera.h"
#include "app_httpd.cpp"
#include "camera_pins.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>

#define CAMERA_MODEL_AI_THINKER
#define LED_BUILTIN 4
#define RELAY_PIN 12 

int enrolled = 0;
int pessoas = 0;
bool lock = false;
const char* ssid = "ESP32-CAM AP";
const char* password = "inesctec";
WiFiServer server(80);


// ------------- HANDLE REQUEST -------------
void handleRequest(WiFiClient client) {
  String requestHeaders = "";
  String requestBody = "";

  while (client.available()) {
    String line = client.readStringUntil('\n');
    requestHeaders += line + '\n';
    if (line == "\r") {
      break;
    }
  }

  while (client.available()) {
    requestBody += (char)client.read();
  }
  // Parse the JSON payload
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, requestBody);

  if (error) {
    Serial.print("Failed to parse JSON payload: ");
    Serial.println(error.c_str());
    return;
  }

  String myString = doc["myString"].as<String>();

  if (myString == "enroll" && enrolled == 0) {
    Serial.println("ONE MORE");
    pessoas++;
    enrolled = 10;

  } else if (myString == "deleteA") {
    Serial.println("Delete all!");
    del_all();
    

  } else if (myString == "deleteL") {
    Serial.println("Delete last!");
    pessoas--;
    delete_face(&id_list);
    Serial.printf("Still %d left!\n", pessoas);
  }
  else if (myString == "lock")
  {
    Serial.println("Locked!");
    lock = true;
  }
  else if (myString == "unlock")
  {
    Serial.println("Unlocked!");
    lock = false;
  }
  else if (myString == "open")
  {
    Serial.println("Door Opened!");
    open();
  }
}
// ------------- HANDLE REQUEST -------------


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // Start AP
  WiFi.softAP(ssid,password);
  server.begin();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  // Camera Init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  face_id_init(&id_list, FACE_ID_SAVE_NUMBER, ENROLL_CONFIRM_TIMES);
}

void loop() {
  WiFiClient client = server.available();
  if (client)
  {
    handleRequest(client);
    client.stop();
  }
  if (!lock)
  { 
    camera_fb_t* fb = esp_camera_fb_get();
      if (fb) {
        uint8_t* out_buf;
        bool s;
        int face_id = 0;
        dl_matrix3du_t* image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
        if (image_matrix) {
          s = fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item);
          if (s) {
            mtmn_config_t mtmn_config;
            mtmn_config.type = NORMAL;
            mtmn_config.min_face = 80;
            mtmn_config.pyramid = 0.707;
            mtmn_config.pyramid_times = 4;
            mtmn_config.p_threshold.score = 0.6;
            mtmn_config.p_threshold.nms = 0.7;
            mtmn_config.p_threshold.candidate_number = 20;
            mtmn_config.r_threshold.score = 0.7;
            mtmn_config.r_threshold.nms = 0.7;
            mtmn_config.r_threshold.candidate_number = 10;
            mtmn_config.o_threshold.score = 0.7;
            mtmn_config.o_threshold.nms = 0.7;
            mtmn_config.o_threshold.candidate_number = 1;
            box_array_t* net_boxes = face_detect(image_matrix, &mtmn_config);
            if (net_boxes)
            // When a face is detected in front of the ESP
            {
              int face_id = 0;
              face_id = run_face_recognition(image_matrix, net_boxes);
            }
          }
        }
        dl_matrix3du_free(image_matrix);
      }
      esp_camera_fb_return(fb);
  }
}
