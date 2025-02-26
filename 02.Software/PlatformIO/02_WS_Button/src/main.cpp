#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define BUTTON_PIN 0
#define LED_PIN 48  // 连接 WS2812B 的引脚
#define LED_COUNT 1 // WS2812B 灯珠的数量

static uint8_t state = 0;     // 全局状态变量

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// 按钮任务
void buttonTask(void *param)
{
  while (1)
  {
    if (digitalRead(BUTTON_PIN) == LOW) // 检测按钮按下
    {
      delay(100);                         // 简单去抖动
      if (digitalRead(BUTTON_PIN) == LOW) // 确认按钮按下
      {
        Serial.println("Button Down");
        if (state < 3)
        {
          state++;
        }
        else
        {
          state = 0;
        }
        Serial.println(state);
      }
      else
      {
        Serial.println("Button UP");
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS); // 稍微延时，避免占用过多 CPU
  }
}

// WS2812B 任务
void wsTask(void *param)
{
  while (1)
  {
    switch (state)
    {
    case 1:
      strip.setPixelColor(0, strip.Color(20, 0, 0)); // 红色
      break;
    case 2:
      strip.setPixelColor(0, strip.Color(0, 20, 0)); // 绿色
      break;
    case 3:
      strip.setPixelColor(0, strip.Color(0, 0, 20)); // 蓝色
      break;
    default:
      strip.setPixelColor(0, strip.Color(0, 0, 0)); // 关闭 LED
      break;
    }
    strip.show();                        // 更新 LED 状态
    vTaskDelay(10 / portTICK_PERIOD_MS); // 稍微延时，避免占用过多 CPU
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP); // 启用上拉电阻

  strip.begin();
  strip.show(); // 初始化时关闭所有灯珠

  // 创建任务
  xTaskCreate(buttonTask, "button", 1024 * 2, NULL, 3, NULL);
  xTaskCreate(wsTask, "ws", 1024 * 2, NULL, 2, NULL);
}

void loop()
{
  // 主循环为空，所有逻辑在任务中处理
}