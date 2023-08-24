///////////////////////////////////////////////////////////////////////////////// includes
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Base64.h"
#include "esp_camera.h"

#include <WebServer.h>
#include <WiFiClient.h>

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

#include "src/OV2640.h"
OV2640 cam;

const char *myDomain = "script.google.com";
String getAll = "", getBody = "";

String script = "/macros/s/AKfycbz6XfQvLQ-AyDTg-bPnlq-oqAevKkw4QBdF0H-MVOkI1D8VwCme6rl7Ynf7SW5uBXN0lQ/exec";
String lineNotifyToken = "token=**********";
String folderName = "&folderName=ESP32cam_images";
String fileName = "&fileName=ESP32-CAM.jpg";
String image = "&file=";

/////////////////////////////////////////////////////////////////////////////////////// interrupt

#define camTrig 14
// boolean buttonState = false;

// void IRAM_ATTR clickImg(void * arg)
//{m
//   if (buttonState == false)
//     buttonState = true;
// }

///////////////////////////////////////////////////////////////////////////////////////// connecting to wifi

// Replace with your network credentials
const char* ssid = "Desktop_Adi";
const char* password = "1234567890";

// Replace with the IP address of your Flask server
const char* server_ip = "172.16.161.35";
const int server_port = 8000;

WebServer server(80);

//IPAddress local_IP(192, 168, 194, 147); //194.147
//// Set your Gateway IP address
//IPAddress gateway(192, 168, 194, 1);
//
//IPAddress subnet(255, 255, 255, 0);
//IPAddress primaryDNS(8, 8, 8, 8);   //optional
//IPAddress secondaryDNS(8, 8, 4, 4); //optional


void connectWiFi()
{
//    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
//    Serial.println("STA Failed to configure");
//  }

  WiFi.begin(ssid, password);
  
  Serial.println(WiFi.macAddress());
  //Serial.println("Connecting");
  long tim ;
  tim = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    //    delay(50);
    //    digitalWrite(12, HIGH);
    //    delay(50);
    //    digitalWrite(12, LOW);
    Serial.print(".");
    if (millis() - tim > 6000)
      ESP.restart();
  }

  // IP ip = WiFi.localIP();

  Serial.println("");
  Serial.print("Connected to WiFi network in " + String(((millis() - tim) / 1000.0)) + " seconds with IP Address: " +WiFi.localIP());
  Serial.println();

  //  Serial.println(F("WiFi connected"));
  //  Serial.println("");
  //  Serial.println(ip);
  Serial.print("Stream Link: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/mjpeg/1");
  server.on("/mjpeg/1", HTTP_GET, handle_jpg_stream);
  //  server.on("/jpg", HTTP_GET, handle_jpg);
  server.onNotFound(handleNotFound);
  server.begin();
}

////////////////////////////////////////////////////////////////////////////////////////////// camera config

void configCam()
{
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
  config.pixel_format = PIXFORMAT_JPEG;

  // init with high specs to pre-allocate larger buffers
  if (psramFound()) {
    config.frame_size = FRAMESIZE_XGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(100);
    ESP.restart();
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void startAP()
{
  IPAddress ip;
  //  WiFi.mode(WIFI_STA);
  //  WiFi.softAP(ssid, password);
  //  ip = WiFi.softAPIP();

  // WiFi.connect(ssid, pass);
  ip = WiFi.localIP();

  //  Serial.println(F("WiFi connected"));
  //  Serial.println("");
  //  Serial.println(ip);
  Serial.print("Stream Link: http://");
  Serial.print(ip);
  Serial.println("/mjpeg/1");
  server.on("/mjpeg/1", HTTP_GET, handle_jpg_stream);
  //  server.on("/jpg", HTTP_GET, handle_jpg);
  //  server.onNotFound(handleNotFound);
  server.begin();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////// setup
WiFiClient client;

void setup()
{
  pinMode(12, OUTPUT);
  pinMode(4, OUTPUT);

  pinMode(camTrig, INPUT_PULLUP);

  Serial.begin(115200);
  //  delay(10);

  configCam();
  connectWiFi();

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
}

////////////////////////////////////////////////////////////////////////////////////////////////////// streaming

const char HEADER[] = "HTTP/1.1 200 OK\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "Content-Type: multipart/x-mixed-replace; boundary=1234567890\r\n";
const char BOUNDARY[] = "\r\n--1234567890\r\n";
const char CTNTTYPE[] = "Content-Type: image/jpeg\r\nContent-Length: ";
const int hdrLen = strlen(HEADER);
const int bdrLen = strlen(BOUNDARY);
const int cntLen = strlen(CTNTTYPE);


String SendCapturedImage(camera_fb_t * f)
{
  camera_fb_t * fb = f;

  if (!fb) {
    Serial.println("Failed to capture image");
    return "";
  }

  // Connect to the Flask server

  if (!client.connect(server_ip, server_port)) {
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


void handle_jpg_stream(void)
{
  char buf[32];
  int s;

  WiFiClient client = server.client();

  client.write(HEADER, hdrLen);
  client.write(BOUNDARY, bdrLen);

  while (true)
  {
    if (!client.connected())
      break;
    cam.run();
    s = cam.getSize();
    camera_fb_t* fb = cam.getframe();
    uint8_t * buffer = fb->buf;
    client.write(CTNTTYPE, cntLen);
    sprintf(buf, "%d\r\n\r\n", s);
    client.write(buf, strlen(buf));
    client.write((char *)buffer, s);
    client.write(BOUNDARY, bdrLen);

    if (!digitalRead(camTrig))
    {
      delay(100);
      if (!digitalRead(camTrig))
      {
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
  }
}

void handleNotFound()
{
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

String urlencode(String str)
{
  String encodedString = "";
  char c;
  char code0;
  char code1;
  char code2;
  for (int i = 0; i < str.length(); i++)
  {
    c = str.charAt(i);
    if (c == ' ')
    {
      encodedString += '+';
    }
    else if (isalnum(c))
    {
      encodedString += c;
    }
    else
    {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9)
      {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9)
      {
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

////////////////////////////////////////////////////////////////////////////////////////////////// image capture


////////////////////////////////////////////////////////////////////////////////// loop

void loop()
{
  server.handleClient();
}
