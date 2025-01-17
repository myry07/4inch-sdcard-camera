#include <TFT_eSPI.h>
#include <Arduino.h>
#include <SPI.h>
#include <lvgl.h>
#include "ui.h"
#include <Adafruit_NeoPixel.h>
#include <TJpg_Decoder.h>
#include "esp_camera.h"
#include "sd_read_write.h"

#define BUTTON_PIN 0

#define LED_PIN 48  // 连接WS2812B的引脚

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

Adafruit_NeoPixel strip(1, LED_PIN, NEO_GRB + NEO_KHZ800);
int led_state = 0;
int camera_state = 0;

TaskHandle_t led_Handle = NULL;
TaskHandle_t camera_Handle = NULL;

/*screen resolution*/
static const uint16_t screenWidth = 480;
static const uint16_t screenHeight = 320;
uint16_t calData[5] = { 353, 3568, 269, 3491, 7 }; /*touch caldata*/

TFT_eSPI tft = TFT_eSPI(); /* TFT entity */

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[screenWidth * screenHeight / 10];  // 增加缓冲区大小

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
  config.xclk_freq_hz = 10000000;
  config.frame_size = FRAMESIZE_QVGA;    // 设置为 QQVGA（160x120）
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 16;  // 提高 JPEG 压缩质量
  config.fb_count = 2;

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);  //初始化失败
    return;
  } else {
    Serial.println("Camera init");
  }

  delay(1000);
}

void showingImage() {
  camera_fb_t *fb = esp_camera_fb_get();  // 获取帧缓冲
  if (!fb) {
    Serial.println("Failed to capture image");  // 无法捕获图片
    return;
  }

  if (fb->format != PIXFORMAT_JPEG) {
    Serial.println("no JPEG format");  // 非JPEG
  } else {
    Serial.println("Captured JPEG image");

    int offsetX = 0;   //左右偏移 +右 -左
    int offsetY = 50;  //上下偏移 +下 -上

    Serial.print("Image size: ");
    Serial.print(fb->width);
    Serial.print("x");
    Serial.println(fb->height);

    if (TJpgDec.drawJpg(offsetX, offsetY, (const uint8_t *)fb->buf, fb->len)) {
      Serial.println("JPEG drawn successfully.");
    } else {
      Serial.println("JPEG draw failed.");  // 超出画框
    }

    if (digitalRead(BUTTON_PIN) == LOW) {
      delay(20);
      if (digitalRead(BUTTON_PIN) == LOW) {
        while (digitalRead(BUTTON_PIN) == LOW)
          ;
        int photo_index = readFileNum(SD_MMC, "/camera");
        if (photo_index != -1) {
          String path = "/camera/" + String(photo_index) + ".jpg";
          writejpg(SD_MMC, path.c_str(), fb->buf, fb->len);

          String text = "Picture in SD: " + String(photo_index);
          lv_label_set_text(ui_Label4, text.c_str());
        }
      }
    }
  }
  esp_camera_fb_return(fb);  // 释放缓冲
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

// LVGL 显示刷新函数
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)color_p, w * h, true);
  tft.endWrite();
  lv_disp_flush_ready(disp);
}

// 触摸读取函数
uint16_t touchX, touchY;
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  bool touched = tft.getTouch(&touchX, &touchY, 600);
  if (!touched) {
    data->state = LV_INDEV_STATE_REL;
  } else {
    data->state = LV_INDEV_STATE_PR;
    // 根据屏幕旋转调整触摸坐标
    data->point.x = screenWidth - touchX;  // 水平镜像
    data->point.y = touchY;                // 垂直镜像
  }
}

void lvglInit() {
  Serial.begin(115200);
  Serial.println("Starting setup...");

  // LVGL init
  lv_init();
  Serial.println("LVGL initialized.");

  lv_disp_draw_buf_init(&draw_buf, buf1, NULL, screenWidth * screenHeight / 10);
  Serial.println("Display buffer initialized.");

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  disp_drv.full_refresh = 1;  // 全刷新模式
  lv_disp_drv_register(&disp_drv);

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  // 启用软件旋转
  // disp_drv.sw_rotate = 1;              // 启用软件旋转
  // disp_drv.rotated = LV_DISP_ROT_180;  // 旋转 180 度

  lv_disp_drv_register(&disp_drv);

  tft.fillScreen(TFT_BLACK);

  ui_init();

  int index = readFileNum(SD_MMC, "/camera");
  String text = "Picture in SD: " + String(index);
  lv_label_set_text(ui_Label4, text.c_str());

  Serial.println("Setup done.");
}

void lvglTask(void *param) {
  lvglInit();
  while (1) {
    lv_timer_handler();
    lv_tick_inc(5);                      // 放置5ms过去
    vTaskDelay(5 / portTICK_PERIOD_MS);  // 使用 vTaskDelay 释放 CPU

    //创建相机
    if (camera_state == 1) {
      showingImage();
    }


    //创建灯珠
    if (led_state == 1) {
      if (led_Handle == NULL) {
        xTaskCreate(wsTask, "ws", 1024, NULL, 0, &led_Handle);
      }
    }

    //删除灯珠
    if (led_state == 0) {
      if (led_Handle != NULL) {
        strip.setPixelColor(0, strip.Color(0, 0, 0));
        strip.show();

        lv_slider_set_value(ui_redSlider, 0, LV_ANIM_ON);
        lv_slider_set_value(ui_blueSlider, 0, LV_ANIM_ON);
        lv_slider_set_value(ui_greenSlider, 0, LV_ANIM_ON);

        vTaskDelete(led_Handle);
        led_Handle = NULL;
      }
    }
  }
}

void wsTask(void *param) {
  while (1) {
    int blue = lv_slider_get_value(ui_blueSlider);
    int green = lv_slider_get_value(ui_greenSlider);
    int red = lv_slider_get_value(ui_redSlider);
    strip.setPixelColor(0, strip.Color(red, green, blue));
    strip.show();
    strip.show();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
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
  Serial.begin(115200);
  Serial.println("Starting setup...");

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  sdmmcInit();
  //removeDir(SD_MMC, "/camera");
  createDir(SD_MMC, "/camera");
  listDir(SD_MMC, "/camera", 0);

  strip.begin();
  strip.show();  // 初始化时先关闭所有灯珠

  displayInit();
  cameraInit();

  xTaskCreate(lvglTask, "lvgl", 1024 * 16, NULL, 1, NULL);

  Serial.println("Setup done.");
}

void loop() {}