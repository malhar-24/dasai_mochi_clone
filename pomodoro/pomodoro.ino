#include <Arduino.h>
#include <U8g2lib.h>

// ------------------ DISPLAY + BUZZER ------------------
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);
#define BUZZER 0

// ------------------ POMODORO DATA ------------------
bool isWork = true;       
unsigned long periodSec;
unsigned long startTime;

// Distance between minute marks
#define PIXELS_PER_MIN 12  

// ----------------------------------------------------
void startPeriod() {
  if (isWork) periodSec = 1 * 60;   // 20 minutes
  else        periodSec = 1 * 60;   // 10 minutes

  startTime = millis();
}

// ----------------------------------------------------
void beepTone() {
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

// ----------------------------------------------------
void drawUI() {
  u8g2.clearBuffer();

  // Compute time
  unsigned long elapsed = (millis() - startTime) / 1000;
  if (elapsed > periodSec) elapsed = periodSec;
  unsigned long remain = periodSec - elapsed;

  // ---- Draw center pointer ----
  int centerX = 64;
  u8g2.drawLine(centerX, 15, centerX, 33);   // SAME height

  // ---- SCALE IS MOVED UP ----
  int scaleY_bottom = 28;   // was 35
  int scaleY_top    = 23;   // was 30
  int numberY       = 40;   // was 45 (moved up slightly)

  // Total scale length
  long totalPixels = (periodSec * PIXELS_PER_MIN) / 60;
  long shift = (elapsed * totalPixels) / periodSec;

  // ---- Draw major ticks (minutes) ----
  for (int m = 0; m <= (periodSec / 60); m++) {
    long x = centerX + (m * PIXELS_PER_MIN) - shift;

    if (x < -5 || x > 133) continue;

    // Major tick
    u8g2.drawLine(x, scaleY_bottom, x, scaleY_top);

    // Minute label
    char buf[4];
    sprintf(buf, "%d", m);
    u8g2.setFont(u8g2_font_tom_thumb_4x6_tr);
    u8g2.drawStr(x - 2, numberY, buf);
  }

  // ---- Draw small ticks (every 10 sec) ----
  for (int s = 0; s <= periodSec; s += 10) {
    long x = centerX + (s * PIXELS_PER_MIN) / 60 - shift;

    if (x < -5 || x > 133) continue;

    u8g2.drawPixel(x, scaleY_bottom - 1);
  }

  // ---- Display remaining time ----
  char timeBuf[10];
  sprintf(timeBuf, "%02d:%02d", remain / 60, remain % 60);

  u8g2.setFont(u8g2_font_logisoso16_tf);
  u8g2.drawStr(35, 60, timeBuf);

  // ---- Status label ----
  u8g2.setFont(u8g2_font_6x12_tr);
  if (isWork)
    u8g2.drawStr(2, 10, "WORK");
  else
    u8g2.drawStr(2, 10, "BREAK");

  u8g2.sendBuffer();
}

// ----------------------------------------------------
void setup() {
  pinMode(BUZZER, OUTPUT);
  u8g2.begin();
  startPeriod();
}

// ----------------------------------------------------
void loop() {
  drawUI();

  unsigned long elapsed = (millis() - startTime) / 1000;
  if (elapsed >= periodSec) {
    beepTone();
    isWork = !isWork;
    startPeriod();
  }
}
