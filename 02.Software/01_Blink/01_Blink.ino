#include <Adafruit_NeoPixel.h>

#define LED_PIN 48  // 连接WS2812B的引脚，这里是D48
#define LED_COUNT 1  // WS2812B灯珠的数量，按需修改

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.show(); // 初始化时先关闭所有灯珠
}

void loop() {
  // 设置第一个灯珠为红色并显示
  strip.setPixelColor(0, strip.Color(255, 0, 0));
  strip.show();
  delay(1000);

  // 设置第一个灯珠为绿色并显示
  strip.setPixelColor(0, strip.Color(0, 255, 0));
  strip.show();
  delay(1000);

  // 设置第一个灯珠为蓝色并显示
  strip.setPixelColor(0, strip.Color(0, 0, 255));
  strip.show();
  delay(1000);
}