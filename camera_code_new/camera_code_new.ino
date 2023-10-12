#include <WiFi.h>
#include <WiFiClient.h>
#include "esp_camera.h"
#include <esp_wifi.h>

#include <WebServer.h>

#include <SoftwareSerial.h>

// Define software serial pins
#define RX_PIN 14
#define TX_PIN 15 // not used

SoftwareSerial mySerial(RX_PIN, TX_PIN);

int ip_address_of_flask_server[4];

// Replace with your network credentials
const char* ssid = "Desktop_Adi";
const char* password = "1234567890";

#define camTrig 14

// Replace with the IP address of your Flask server
String server_ip = "";
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
  config.frame_size = FRAMESIZE_SVGA;
  // config.frame_size = FRAMESIZE_UXGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Failed to initialize camera (error=%d)", err);
    while (true) {}
  }
}


// confirming IP Address of flask server
WebServer server(80);

WiFiClient client;

///////////////////////////////////////////////////////////////////////////////////////////
const char HEADER[] = "HTTP/1.1 200 OK\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "Content-Type: multipart/x-mixed-replace; boundary=1234567890\r\n";
const char BOUNDARY[] = "\r\n--1234567890\r\n";
const char CTNTTYPE[] = "Content-Type: image/jpeg\r\nContent-Length: ";
const int hdrLen = strlen(HEADER);
const int bdrLen = strlen(BOUNDARY);
const int cntLen = strlen(CTNTTYPE);


String SendCapturedImage(camera_fb_t* f) {
  camera_fb_t* fb = f;

  if (!fb) {
    Serial.println("Failed to capture image");
    return "";
  }

  // Connect to the Flask server

  if (!client.connect(server_ip.c_str(), server_port)) {
    Serial.println("Failed to connect to server");
    esp_camera_fb_return(fb);
    return "";
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
  Serial.println("done1");
  String response = client.readStringUntil('\n');
  Serial.println(response);
  esp_camera_fb_return(fb);
  Serial.println("done2");
  return "";
}


void handle_jpg_stream(void) {
  char buf[32];
  int s;

  WiFiClient client = server.client();

  client.write(HEADER, hdrLen);
  client.write(BOUNDARY, bdrLen);

  while (true) {
    if (!client.connected())
      break;

    camera_fb_t* fb;

    if (fb)
      //return the frame buffer back to the driver for reuse
      esp_camera_fb_return(fb);

    fb = esp_camera_fb_get();

    if (!fb)
      s = 0;  // FIXME - this shouldn't be possible but apparently the new cam board returns null sometimes?
    s = fb->len;

    uint8_t* buffer = fb->buf;
    client.write(CTNTTYPE, cntLen);
    sprintf(buf, "%d\r\n\r\n", s);
    client.write(buf, strlen(buf));
    client.write((char*)buffer, s);
    client.write(BOUNDARY, bdrLen);

    if (!digitalRead(camTrig)) {
      delay(100);
      if (!digitalRead(camTrig)) {
        //        sensor_t * s = esp_camera_sensor_get();

        //        sensor_t * sen = esp_camera_sensor_get();
        //
        //        sen->set_framesize(sen, FRAMESIZE_CIF);
        //        sen->set_quality(sen, 10);
        //                s->set_dcw(s, 0);
        Serial.println("Sending Image");
        SendCapturedImage(fb);

        //        sen->set_framesize(sen, FRAMESIZE_UXGA);
        //        sen->set_quality(sen, 10);
        //                s->set_dcw(s, 1);
      }
    }
    esp_camera_fb_return(fb);
  }
}

void handleNotFound() {
  String message = "Server is running!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  server.send(200, "text / plain", message);
}

String urlencode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  char code2;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    } else if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      code2 = '\0';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
      // encodedString+=code2;
    }
    yield();
  }
  return encodedString;
}

////////////////////////////////////////////////////////////////////////////////////////////


void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);  // SoftwareSerial initialization
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

  Serial.println(localIP.toString());

  int ipcount = 0;

  while (ipcount < 4) {

    int receivedValue = mySerial.parseInt();  // Read the incoming integer value
    if (receivedValue >= 255) {
      receivedValue = receivedValue - 255;
      ip_address_of_flask_server[ipcount] = receivedValue;
      ipcount++;
    }
  }

  mySerial.end();
  server_ip = String(ip_address_of_flask_server[0]) + "." + String(ip_address_of_flask_server[1]) + "." + String(ip_address_of_flask_server[2]) + "." + String(ip_address_of_flask_server[3]);
  Serial.println(server_ip);


  pinMode(camTrig, INPUT_PULLUP);

  server.on("/mjpeg/1", HTTP_GET, handle_jpg_stream);
  //  server.on("/jpg", HTTP_GET, handle_jpg);
  server.onNotFound(handleNotFound);
  server.begin();

  // Serial.println("IP Recieved: " + server_ip);

  while (true) {
    // Serial.println("1");

    Serial.println(server_ip + " " + server_port);

    if (client.connect(server_ip.c_str(), server_port)) {
      Serial.println("connected to server");

      client.println("POST /save_ip HTTP/1.1");
      client.println("Host: " + server_ip + ":" + String(server_port));
      client.println("Content-Type: text/plain");
      client.println("Content-Length: " + String((localIP.toString() + "/mjpeg/1").length()));
      client.println();
      client.println(localIP.toString() + "/mjpeg/1");

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
  Serial.println("Before cam init");
  init_camera();
  
  Serial.println("After cam init");
}

void loop() {
  server.handleClient();
}
