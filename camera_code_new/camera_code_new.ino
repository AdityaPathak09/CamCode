#include <WiFi.h>
#include <WiFiClient.h>
#include "esp_camera.h"

// Replace with your network credentials
const char* ssid = "Desktop_Adi";
const char* password = "1234567890";

// Replace with the IP address of your Flask server
const char* server_ip = "localhost";
const int server_port = 8000;

// Initialize the camera
camera_config_t config;
void init_camera() {
  // Set camera settings
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
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_UXGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Failed to initialize camera (error=%d)", err);
    while (true) {}
  }
}

WiFiClient client;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Connect to WiFi network
  Serial.printf("Connecting to %s\n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  IPAddress localIP = WiFi.localIP();

  while (true) {
    if (client.connect(server_ip, server_port)) {
      Serial.println("connected to server");

      client.println("POST /save_ip HTTP/1.1");
      client.println("Host: " + String(server_ip) + ":" + String(server_port));
      client.println("Content-Type: text/plain");
      client.println("Content-Length: " + String((localIP.toString()+"/mjpeg/1").length()));
      client.println();
      client.println(localIP.toString()+"/mjpeg/1");

      while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          break;
        }
      }
      String response = client.readStringUntil('\n');
      Serial.println(response);

      break;
    }
    Serial.println("in loop");
  }

  // Initialize the camera
  init_camera();
}

void loop() {
  // Capture an image from the camera
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Failed to capture image");
    return;
  }

  // Connect to the Flask server

  if (!client.connect(server_ip, server_port)) {
    Serial.println("Failed to connect to server");
    esp_camera_fb_return(fb);
    return;
  }

  // Send the image data to the Flask server

  client.println("POST /save_image HTTP/1.1");
  client.println("Host: " + String(server_ip) + ":" + String(server_port));
  client.println("Content-Type: image/jpeg");
  client.println("Content-Length: " + String(fb->len));
  client.println();
  client.write(fb->buf, fb->len);
  client.println();

  // Wait for the server to respond
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  String response = client.readStringUntil('\n');
  Serial.println(response);

  // Cleanup
  //  client.stop();
  esp_camera_fb_return(fb);

  // Wait for some time before capturing the next image
  delay(1000);
}
