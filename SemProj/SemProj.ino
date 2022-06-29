#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Base64.h"

#include "esp_camera.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* ssid     = "realme 3 Pro";   //your network SSID
const char* password = "1234567890";   //your network password
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
String script = "/macros/s/AKfycbzXOik8kF2lrK5JwEBeUTlDYywq-8xwWQBmIlJbx3rKm-Gp5oPj/exec";
String lineNotifyToken = "token=**********";
String folderName = "&folderName=ESP32cam_images";
String fileName = "&fileName=ESP32-CAM.jpg";
String image = "&file=";

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void connectWiFi()
{
  WiFi.begin(ssid, password);
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

  Serial.println("");
  Serial.print("Connected to WiFi network in " + String(((millis() - tim) / 1000.0)) + " seconds with IP Address: ");
  Serial.println(WiFi.localIP());
}

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
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 8;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 6;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(100);
    ESP.restart();
  }

}

void setup()
{

  pinMode(12, OUTPUT);
  //  digitalWrite(12, HIGH);
  //  delay(100);
  //  digitalWrite(12, LOW);

  pinMode(4, OUTPUT);

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  //  delay(10);

  connectWiFi();
  configCam();
}

void loop()
{
  SendCapturedImage();
  delay(10000);
}

uint8_t* fb;

String SendCapturedImage() {
  const char* myDomain = "script.google.com";
  String getAll = "", getBody = "";

  sensor_t *s = esp_camera_sensor_get();
  s->set_brightness(s, 2);
  s->set_contrast(s, 0);
  s->set_wb_mode(s, 0);
  s->set_exposure_ctrl(s, 1);
  s->set_lenc(s, 1);

  camera_fb_t * fba = NULL;

  digitalWrite(4, HIGH);
  delay(100);

  fb=(uint8_t*) ps_malloc(100000*sizeof(uint8_t));
  fba = esp_camera_fb_get();
  fb = fba->buf;
  
  digitalWrite(4, LOW);
  
  if (!fb) {
    Serial.println("Camera capture failed");
    //delay(1000);
    ESP.restart();
    return "Camera capture failed";
  }

  Serial.println("Connect to " + String(myDomain));
  WiFiClientSecure client_tcp;
  client_tcp.setInsecure();   //run version 1.0.5 or above

  if (client_tcp.connect(myDomain, 443)) {
    Serial.println("Connection successful");

    char *input = (char *)fba->buf;
    char output[base64_enc_len(3)];
    String imageFile = "data:image/jpeg;base64,";
    for (int i = 0; i < fba->len; i++) {
      base64_encode(output, (input++), 3);
      if (i % 3 == 0) imageFile += urlencode(String(output));
    }
    String Data = lineNotifyToken + folderName + fileName + image;

    client_tcp.println("POST " + script + " HTTP/1.1");
    client_tcp.println("Host: " + String(myDomain));
    client_tcp.println("Content-Length: " + String(Data.length() + imageFile.length()));
    client_tcp.println("Content-Type: application/x-www-form-urlencoded");
    client_tcp.println("Connection: keep-alive");
    client_tcp.println();

    client_tcp.print(Data);
    int Index;
    for (Index = 0; Index < imageFile.length(); Index = Index + 1000) {
      client_tcp.print(imageFile.substring(Index, Index + 1000));
    }
    esp_camera_fb_return(fba);

    int waitTime = 500;   // timeout 10 seconds
    long startTime = millis();
    boolean state = false;

    while ((startTime + waitTime) > millis())
    {
      Serial.print(".");
      delay(10);
      while (client_tcp.available())
      {
        char c = client_tcp.read();
        if (state == true) getBody += String(c);
        if (c == '\n')
        {
          if (getAll.length() == 0) state = true;
          getAll = "";
        }
        else if (c != '\r')
          getAll += String(c);
        startTime = millis();
      }
      if (getBody.length() > 0) break;
    }
    client_tcp.stop();
    Serial.println(getBody);
  }
  else {
    getBody = "Connected to " + String(myDomain) + " failed.";
    Serial.println("Connected to " + String(myDomain) + " failed.");
  }

  return getBody;
}


String urlencode(String str)
{
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
      //encodedString+=code2;
    }
    yield();
  }
  return encodedString;
}
