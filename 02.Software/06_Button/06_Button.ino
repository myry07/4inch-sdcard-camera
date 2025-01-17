#define BUTTON_PIN 0

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);  //启用上拉电阻
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(200);
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("I'm Button");
    }
  }
}