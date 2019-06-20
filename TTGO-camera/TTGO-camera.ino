#include <WiFi.h>
#include <OneButton.h>
#include "freertos/event_groups.h"
#include <Wire.h>
#include <Adafruit_BME280.h>
#include "esp_camera.h"
#include "esp_wifi.h"


#define ENABLE_SSD1306
//#define SOFTAP_MODE       //The comment will be connected to the specified ssid
//#define ENABLE_BME280

#define WIFI_SSID   "GalaxyHotspot"
#define WIFI_PASSWD "4807470998"

#define PWDN_GPIO_NUM 26
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 32
#define SIOD_GPIO_NUM 13
#define SIOC_GPIO_NUM 12

#define Y9_GPIO_NUM 39
#define Y8_GPIO_NUM 36
#define Y7_GPIO_NUM 23
#define Y6_GPIO_NUM 18
#define Y5_GPIO_NUM 15
#define Y4_GPIO_NUM 4
#define Y3_GPIO_NUM 14
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 27
#define HREF_GPIO_NUM 25
#define PCLK_GPIO_NUM 19

#define I2C_SDA 21
#define I2C_SCL 22

#ifdef ENABLE_SSD1306
#include "SSD1306.h"
#include "OLEDDisplayUi.h"
#define SSD1306_ADDRESS 0x3c
SSD1306 oled(SSD1306_ADDRESS, I2C_SDA, I2C_SCL);
OLEDDisplayUi ui(&oled);
#endif

#define AS312_PIN 33
#define BUTTON_1 34
String ip;
EventGroupHandle_t evGroup;

OneButton button1(BUTTON_1, true);

#ifdef ENABLE_BME280
#define BEM280_ADDRESS 0X77
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme;
#endif

void startCameraServer();

void button1Func()
{
    static bool en = false;
    xEventGroupClearBits(evGroup, 1);
    sensor_t *s = esp_camera_sensor_get();
    en = en ? 0 : 1;
    s->set_vflip(s, en);
    delay(200);
    xEventGroupSetBits(evGroup, 1);
}

#ifdef ENABLE_SSD1306
void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
    display->setFont(ArialMT_Plain_16);
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->drawString(64 + x, 35 + y, ip);
    #ifndef SOFTAP_MODE
    display->drawString(64 + x, 10 + y, WIFI_SSID);
    #else
    display->drawString(64 + x, 10 + y, buff);
    #endif
    /*if (digitalRead(AS312_PIN)) {
        display->drawString(64 + x, 10 + y, WIFI_SSID);
    }*/
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
#ifdef ENABLE_BME280
    static String temp, pressure, altitude, humidity;
    static uint64_t lastMs;

    if (millis() - lastMs > 2000) {
        lastMs = millis();
        temp = "Temp:" + String(bme.readTemperature()) + " *C";
        pressure = "Press:" + String(bme.readPressure() / 100.0F) + " hPa";
        altitude = "Altitude:" + String(bme.readAltitude(SEALEVELPRESSURE_HPA)) + " m";
        humidity = "Humidity:" + String(bme.readHumidity()) + " %";
    }
    display->setFont(ArialMT_Plain_16);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->drawString(0 + x, 0 + y, temp);
    display->drawString(0 + x, 16 + y, pressure);
    display->drawString(0 + x, 32 + y, altitude);
    display->drawString(0 + x, 48 + y, humidity);
#endif
}

FrameCallback frames[] = {drawFrame1, drawFrame2};
#define FRAMES_SIZE (sizeof(frames) / sizeof(frames[0]))
#endif

void setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();

    pinMode(AS312_PIN, INPUT);

#ifdef ENABLE_SSD1306
    oled.init();
    oled.setFont(ArialMT_Plain_16);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    delay(50);
    oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, "TTGO Camera");
    oled.display();
#endif

    if (!(evGroup = xEventGroupCreate())) {
        Serial.println("evGroup Fail");
        while (1);
    }
    xEventGroupSetBits(evGroup, 1);

#ifdef ENABLE_BME280
    Wire.begin(I2C_SDA, I2C_SCL);
    oled.clear();
    oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, "BME280 Active");
    oled.display();
    if (!bme.begin(BEM280_ADDRESS)) {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        oled.clear();
        oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, "BME280 InActive");
        oled.display();
        while (1);
    }
#endif

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
    //init with high specs to pre-allocate larger buffers
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;

    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init Fail");
#ifdef ENABLE_SSD1306
        oled.clear();
        oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, "Camera init Fail");
        oled.display();
#endif
        while (1);
    }

    //drop down frame size for higher initial frame rate
    sensor_t *s = esp_camera_sensor_get();
    s->set_framesize(s, FRAMESIZE_QVGA);

    button1.attachClick(button1Func);

#ifdef SOFTAP_MODE
    uint8_t mac[6];
    char buff[128];
    WiFi.mode(WIFI_AP);
    IPAddress apIP = IPAddress(2, 2, 2, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    esp_wifi_get_mac(WIFI_IF_AP, mac);
    sprintf(buff, "TTGO-CAMERA-%02X:%02X", mac[4], mac[5]);
    Serial.printf("Device AP Name:%s\n", buff);
    if (!WiFi.softAP(buff, NULL, 1, 0)) {
        Serial.println("AP Begin Failed.");
        while (1);
    }
#else
    WiFi.begin(WIFI_SSID, WIFI_PASSWD);
    Serial.println(oled.getWidth());
    Serial.print(oled.getHeight());
    float tl[] = {oled.getWidth()/3, oled.getHeight()/2};
    float tr[] = {oled.getWidth()*2/3, oled.getHeight()/2};
    float bot[] = {oled.getWidth()/2, oled.getHeight()};
    float bl[] = {oled.getWidth()/3, oled.getHeight()*3/4};
    float br[] = {oled.getWidth()*2/3, oled.getHeight()*3/4};
    float p1[2];
    float p2[2];
    float m;
    float b;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        #ifdef ENABLE_SSD1306
          oled.clear();
          oled.drawString(oled.getWidth() / 2, 0, "Searching for WiFi");
          oled.drawString(oled.getWidth() / 2, oled.getHeight() / 3, WIFI_SSID);
          oled.display();
          delay(500);
          /*
          for (int i = 1; i<=5;i++){
            Serial.print(i);
            switch(i){
              case 1:
                memcpy(p1,tl,sizeof(tl));
                memcpy(p2,bot,sizeof(bot));
                #define L1
                m = (p2[1]-p1[1])/(p2[0]-p1[0]);
                b = p1[1] - m*p1[0];
                for (int x = p1[0], y = p1[1]; x<=p2[0]&&y<=p2[1]; x=x+1, y=m*x+b){
                  oled.drawString(x, y, ".");
                  oled.display();
                  delay(50);
                } 
                break;
              case 2:
                memcpy(p1,bot,sizeof(bot));
                memcpy(p2,tr,sizeof(tr));
                #define L2
                m = (p2[1]-p1[1])/(p2[0]-p1[0]);
                b = p1[1] - m*p1[0];
                for (int x = p1[0], y = p1[1]; x<=p2[0]&&y>=p2[1]; x=x+1, y=m*x+b){
                  oled.drawString(x, y, ".");
                  oled.display();
                  delay(50);
                }
                break;
              case 3:
                memcpy(p1,tr,sizeof(tr));
                memcpy(p2,bl,sizeof(bl));
                #define L3
                m = (p2[1]-p1[1])/(p2[0]-p1[0]);
                b = p1[1] - m*p1[0];
                for (int x = p1[0], y = p1[1]; x>=p2[0]&&y<=p2[1]; x=x+1, y=m*x+b){
                  oled.drawString(x, y, ".");
                  oled.display();
                  delay(50);
                }
                break;
              case 4:
                memcpy(p1,bl,sizeof(bl));
                memcpy(p2,br,sizeof(br));
                #define L4
                m = (p2[1]-p1[1])/(p2[0]-p1[0]);
                b = p1[1] - m*p1[0];
                for (int x = p1[0], y = p1[1]; x<=p2[0]&&y<=p2[1]; x=x+1, y=m*x+b){
                  oled.drawString(x, y, ".");
                  oled.display();
                  delay(50);
                }
                break;
              case 5:
                memcpy(p1,br,sizeof(br));
                memcpy(p2,tl,sizeof(tl));
                #define L5
                m = (p2[1]-p1[1])/(p2[0]-p1[0]);
                b = p1[1] - m*p1[0];
                for (int x = p1[0], y = p1[1]; x>=p2[0]&&y>=p2[1]; x=x+1, y=m*x+b){
                  oled.drawString(x, y, ".");
                  oled.display();
                  delay(50);
                }
                break;
            }    
          }
          */
          oled.drawString(oled.getWidth() / 3, oled.getHeight() / 2, ".");
          oled.display();
          delay(500);
          oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, ".");
          oled.display();
          delay(500);
          oled.drawString(2*oled.getWidth() / 3, oled.getHeight() / 2, ".");
          oled.display();
          delay(500);
        #endif
    }
    Serial.println("");
    Serial.println("WiFi connected");
    
    #ifdef ENABLE_SSD1306
      oled.clear();
      oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, "WiFi Connected");
      oled.display();
      delay(500);
    #endif
    
#endif


    startCameraServer();

    delay(50);

#ifdef ENABLE_SSD1306
    ui.setTargetFPS(60);
    ui.disableIndicator();
    //ui.setIndicatorPosition(BOTTOM);
    //ui.setIndicatorDirection(LEFT_RIGHT);
    //ui.setFrameAnimation(SLIDE_LEFT);
    ui.setFrames(frames, FRAMES_SIZE);
    ui.disableAutoTransition();
    ui.setTimePerFrame(6000);
    ui.init();
#endif

#ifdef SOFTAP_MODE
    ip = WiFi.softAPIP().toString();
    Serial.printf("\nAp Started .. Please Connect %s hotspot\n", buff);
#else
    ip = WiFi.localIP().toString();
#endif

    Serial.print("Camera Ready! Use 'http://");
    Serial.print(ip);
    Serial.println("' to connect");
}

void loop()
{
#ifdef ENABLE_SSD1306
    if (ui.update()) {
#endif
        button1.tick();
#ifdef ENABLE_SSD1306
    }
#endif
}