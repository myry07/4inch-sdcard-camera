#include <TFT_eSPI.h>
#include <Arduino.h>
#include <SPI.h>
#include <lvgl.h>
#include "ui.h"
#include <Adafruit_NeoPixel.h>

#define LED_PIN 48   // 连接WS2812B的引脚，这里是D48
#define LED_COUNT 1  // WS2812B灯珠的数量，按需修改

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

/*screen resolution*/
static const uint16_t screenWidth = 480;
static const uint16_t screenHeight = 320;
uint16_t calData[5] = { 353, 3568, 269, 3491, 7 }; /*touch caldata*/

TFT_eSPI tft = TFT_eSPI(); /* TFT entity */

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[screenWidth * screenHeight / 10];  // 增加缓冲区大小

int temp = 0;

//_______________________
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)color_p, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

uint16_t touchX, touchY;
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  bool touched = tft.getTouch(&touchX, &touchY, 600);
  if (!touched) {
    data->state = LV_INDEV_STATE_REL;
  } else {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = touchX;
    data->point.y = touchY;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting setup...");

  strip.begin();
  strip.show();  // 初始化时先关闭所有灯珠

  // LCD init
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTouch(calData);
  Serial.println("LCD initialized.");

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
  disp_drv.sw_rotate = 1;              // 启用软件旋转
  disp_drv.rotated = LV_DISP_ROT_180;  // 旋转 180 度

  lv_disp_drv_register(&disp_drv);

  tft.fillScreen(TFT_BLACK);

  ui_init();
  Serial.println("Setup done.");
}

void loop() {
  lv_timer_handler();
  lv_tick_inc(5);  // 放置5ms过去
  delay(5);

  Serial.println(temp);
  
  if (temp == 1) {
    strip.setPixelColor(0, strip.Color(255, 0, 0));
    strip.show();
    delay(1000);

    strip.setPixelColor(0, strip.Color(0, 255, 0));
    strip.show();
    delay(1000);

    strip.setPixelColor(0, strip.Color(0, 0, 255));
    strip.show();
    delay(1000);
  } else {
    strip.setPixelColor(0, strip.Color(0, 0, 0));
  }
}