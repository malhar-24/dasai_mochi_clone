#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

#define BTN_INC 2
#define BTN_OK  1
#define BUZZER  0   // 🔥 Buzzer pin

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

int d[4] = {0,0,0,0};
int pos = 0;
bool timerRunning = false;

unsigned long lastTick = 0;

bool readButton(int pin) {
  return digitalRead(pin) == LOW;
}

void beepFinish() {
  // 🔥 Beep 3 times
  tone(BUZZER, 1500, 200);
  delay(200);
  tone(BUZZER, 1800, 200);
  delay(200);
  tone(BUZZER, 1500, 200);
  delay(200);
  tone(BUZZER, 1800, 200);
  delay(200);
  tone(BUZZER, 1500, 200);
  delay(200);
  tone(BUZZER, 1800, 200);
  delay(200);
  tone(BUZZER, 1500, 200);
  delay(200);
  tone(BUZZER, 1800, 200);
  delay(200);
}

void drawTimer() {
  char buffer[6];
  sprintf(buffer, "%d%d:%d%d", d[0], d[1], d[2], d[3]);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_logisoso22_tf);

  int textWidth = u8g2.getStrWidth(buffer);
  int xStart = (128 - textWidth) / 2;
  int yBase = 42;

  u8g2.drawStr(xStart, yBase, buffer);

  int wDigit = u8g2.getStrWidth("8");
  int wColon = u8g2.getStrWidth(":");

  int x0 = xStart;
  int x1 = x0 + wDigit;
  int xColon = x1 + wDigit;
  int x2 = xColon + wColon;
  int x3 = x2 + wDigit;

  int digitX[4] = { x0, x1, x2, x3 };

  if (!timerRunning) {
    u8g2.drawBox(digitX[pos], yBase + 4, wDigit, 3);
  }

  u8g2.sendBuffer();
}

void decreaseTimer() {
  if (millis() - lastTick < 1000) return;
  lastTick = millis();

  int totalSec =
    d[0]*600 + d[1]*60 + d[2]*10 + d[3];

  if (totalSec <= 0) {
    timerRunning = false;
    beepFinish();      // 🔥 Buzzer added here
    return;
  }

  totalSec--;

  d[0] = totalSec / 600;
  totalSec %= 600;
  d[1] = totalSec / 60;
  totalSec %= 60;
  d[2] = totalSec / 10;
  d[3] = totalSec % 10;
}

void setup() {
  pinMode(BTN_INC, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);   // 🔥 Setup buzzer

  digitalWrite(BUZZER, LOW);

  u8g2.begin();
  drawTimer();
}

void loop() {

  if (!timerRunning) {

    if (readButton(BTN_INC)) {
      d[pos]++;
      if (d[pos] > 9) d[pos] = 0;
      drawTimer();
      delay(250);
    }

    if (readButton(BTN_OK)) {
      pos++;
      if (pos > 3) {
        pos = 0;
        timerRunning = true;  // Start countdown
      }
      drawTimer();
      delay(250);
    }

  } else {
    decreaseTimer();
    drawTimer();
  }
}
