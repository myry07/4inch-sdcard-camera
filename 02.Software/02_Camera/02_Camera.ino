#include "esp_camera.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>

#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 15
#define SIOD_GPIO_NUM 4
#define SIOC_GPIO_NUM 5

#define Y2_GPIO_NUM 11
#define Y3_GPIO_NUM 9
#define Y4_GPIO_NUM 8
#define Y5_GPIO_NUM 10
#define Y6_GPIO_NUM 12
#define Y7_GPIO_NUM 18
#define Y8_GPIO_NUM 17
#define Y9_GPIO_NUM 16

#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM 7
#define PCLK_GPIO_NUM 13

TFT_eSPI tft = TFT_eSPI();

void cameraInit() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

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
  config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 2;

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);  //初始化失败
    return;
  } else {
    Serial.println("Camera init");
  }

  delay(3000);
}

void showingImage() {
  camera_fb_t* fb = esp_camera_fb_get();  // 获取帧缓冲
  if (!fb) {
    Serial.println("Failed to capture image");  // 无法捕获图片
    return;
  }

  if (fb->format != PIXFORMAT_JPEG) {
    Serial.println("no JPEG format");  // 非JPEG
  } else {
    Serial.println("Captured JPEG image");

    int offsetX = 0;   //左右偏移 +右 -左
    int offsetY = 40;  //上下偏移 +下 -上

    if (TJpgDec.drawJpg(offsetX, offsetY, (const uint8_t*)fb->buf, fb->len)) {
      Serial.println("JPEG drawn successfully.");
    } else {
      Serial.println("JPEG draw failed.");  // 超出画框
    }
  }

  esp_camera_fb_return(fb);  // 释放缓冲
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;

  // 计算旋转后的坐标
  // int16_t rotatedX = tft.width() - x - w;  // 水平翻转
  // int16_t rotatedY = tft.height() - y - h; // 垂直翻转

  // 绘制旋转后的图像
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}


void displayInit() {
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  cameraInit();
  displayInit();
}

void loop() {
  // put your main code here, to run repeatedly:
  showingImage();
}