#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

#define BTN_JUMP  2
#define BTN_OK    1
#define BUZZER    0

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ---------------- SCALE ----------------
const int SCALE = 2;  // 1 integer scale to avoid line artifacts

// ---------------- GAME CONSTANTS -------------
const int groundY = 63; //60
const int jumpPeak = groundY - 65; //40

const int dinoOrigW = 15;
const int dinoOrigH = 17;
const int dinoW = dinoOrigW * SCALE;
const int dinoH = dinoOrigH * SCALE;

int dinoY = groundY;
int velocity = 0;
bool inAir = false;

// ---------------- ORIGINAL BITMAPS ----------------
const uint8_t dinoBitmap[17][15] PROGMEM = {
  {0,0,0,0,0,0,0,0,1,1,1,1,1,0,0},
  {0,0,0,0,0,0,0,1,1,1,1,1,1,1,1},
  {0,0,0,0,0,0,0,1,1,0,1,1,1,1,1},
  {0,0,0,0,0,0,0,1,1,1,1,1,1,1,1},
  {0,0,0,0,0,0,0,1,1,1,1,1,0,0,0},
  {0,0,0,0,0,0,0,1,1,1,1,1,1,1,0},
  {1,1,0,0,0,0,1,1,1,1,0,0,0,0,0},
  {1,0,0,0,0,1,1,1,1,1,0,0,0,0,0},
  {1,0,0,1,1,1,1,1,1,1,1,1,1,0,0},
  {1,0,1,1,1,1,1,1,1,1,0,1,1,0,0},
  {1,1,1,1,1,1,1,1,1,1,0,0,0,0,0},
  {1,1,1,1,1,1,1,1,1,1,0,0,0,0,0},
  {0,1,1,1,1,1,1,1,1,0,0,0,0,0,0},
  {0,0,1,1,1,1,1,1,0,0,0,0,0,0,0},
  {0,0,0,1,1,0,1,1,0,0,0,0,0,0,0},
  {0,0,0,1,0,0,1,0,0,0,0,0,0,0,0},
  {0,0,0,1,1,0,1,1,0,0,0,0,0,0,0}
};

// ---- CACTUS MATRICES ----
const uint8_t cactus1[15][10] PROGMEM = {
  {0,0,0,0,1,1,0,0,0,0},{0,0,0,1,1,1,1,0,0,0},{0,0,0,1,1,1,1,0,0,0},
  {0,0,0,1,1,1,1,0,0,0},{0,0,0,1,1,1,1,0,1,1},{0,0,0,1,1,1,1,0,1,1},
  {0,0,0,1,1,1,1,1,1,1},{1,0,0,1,1,1,1,1,1,0},{1,0,0,1,1,1,1,0,0,0},
  {1,1,1,1,1,1,1,0,0,0},{1,1,1,1,1,1,1,0,0,0},{0,0,0,1,1,1,1,0,0,0},
  {0,0,0,1,1,1,1,0,0,0},{0,0,0,1,1,1,1,0,0,0},{0,0,0,1,1,1,1,0,0,0}
};

const uint8_t cactus2[11][10] PROGMEM = {
  {0,0,0,0,1,1,0,0,0,0},{0,1,0,1,1,1,1,1,0,0},{1,1,0,1,1,1,1,1,0,0},
  {1,1,0,1,1,1,1,1,0,0},{1,1,1,1,1,1,1,1,1,0},{1,1,1,1,1,1,1,1,1,1},
  {0,0,0,1,1,1,1,1,0,1},{0,0,0,1,1,1,1,1,0,0},{0,0,0,1,1,1,1,1,0,0},
  {0,0,0,1,1,1,1,1,0,0},{0,0,0,1,1,1,1,1,0,0}
};

const uint8_t cactus3[12][7] PROGMEM = {
  {0,0,1,1,1,0,0},{0,0,1,1,1,0,0},{1,0,1,1,1,0,0},{1,1,1,1,1,0,1},
  {0,1,1,1,1,0,1},{0,0,1,1,1,0,1},{0,0,1,1,1,1,1},{0,0,1,1,1,1,1},
  {0,0,1,1,1,0,0},{0,0,1,1,1,0,0},{0,0,1,1,1,0,0},{0,0,1,1,1,0,0}
};

// ---- STRUCT ----
struct CactusType {
  const uint8_t *bitmap;
  int h, w;
};

CactusType cactusShapes[3] = {
  {(const uint8_t*)cactus1, 15, 10},
  {(const uint8_t*)cactus2, 11, 10},
  {(const uint8_t*)cactus3, 12, 7}
};

int cactusIndex = 0;
int obsX = 128;
bool gameOver = false;

// ---------- Draw scaled block ----------
void drawScaledPixel(int x, int y) {
  for (int dx = 0; dx < SCALE; dx++) {
    for (int dy = 0; dy < SCALE; dy++) {
      u8g2.drawPixel(x + dx, y + dy);
    }
  }
}

// ---------- Draw Dino ----------
void drawDino() {
  for (int y = 0; y < dinoOrigH; y++) {
    for (int x = 0; x < dinoOrigW; x++) {
      if (pgm_read_byte(&dinoBitmap[y][x])) {
        drawScaledPixel(10 + x*SCALE, (dinoY - dinoH) + y*SCALE);
      }
    }
  }
}

// ---------- Draw Cactus ----------
void drawCactus() {
  const CactusType &C = cactusShapes[cactusIndex];
  for (int y = 0; y < C.h; y++) {
    for (int x = 0; x < C.w; x++) {
      uint8_t px = pgm_read_byte((const uint8_t*)C.bitmap + y*C.w + x);
      if (px) {
        drawScaledPixel(obsX + x*SCALE, (groundY - C.h*SCALE) + y*SCALE);
      }
    }
  }
}

// ---------- Draw Ground ----------
void drawGround() {
  u8g2.drawHLine(0, groundY, 128);
}

// ---------- Collision ----------
bool checkCollision() {
  int dinoX1 = 10;
  int dinoX2 = dinoX1 + dinoW;
  int dinoY1 = dinoY - dinoH;
  int dinoY2 = dinoY;

  const CactusType &C = cactusShapes[cactusIndex];
  int obsX1 = obsX;
  int obsX2 = obsX + C.w*SCALE;
  int obsY1 = groundY - C.h*SCALE;
  int obsY2 = groundY;

  return !(dinoX2 < obsX1 || dinoX1 > obsX2 ||
           dinoY2 < obsY1 || dinoY1 > obsY2);
}

// ---------- Reset ----------
void resetGame() {
  dinoY = groundY;
  velocity = 0;
  inAir = false;
  obsX = 128;
  cactusIndex = random(0, 3);
  gameOver = false;
}

// ---------- Buzzer ----------
void beep(int f, int d) {
  tone(BUZZER, f, d);
}

// ---------- Setup ----------
void setup() {
  pinMode(BTN_JUMP, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  u8g2.begin();
  randomSeed(analogRead(A0));

  resetGame();
}

// ---------- Main Loop ----------
void loop() {

  if (!gameOver) {

    // Jump
    if (!inAir && digitalRead(BTN_JUMP) == LOW) {
      velocity = -11;
      inAir = true;
      beep(1500, 40);
    }

    // Gravity
    if (inAir) {
      dinoY += velocity;
      velocity++;

      if (dinoY < jumpPeak) {
        dinoY = jumpPeak;
        velocity = 2;
      }

      if (dinoY >= groundY) {
        dinoY = groundY;
        inAir = false;
      }
    }

    // Move cactus
    obsX -= 4;
    if (obsX < -50) { // leave enough margin for big cactus
      obsX = 128;
      cactusIndex = random(0, 3);
    }

    // Collision
    if (checkCollision()) {
      beep(200, 300);
      gameOver = true;
    }

    // Draw frame
    u8g2.clearBuffer();
    drawGround();
    drawDino();
    drawCactus();
    u8g2.sendBuffer();

    delay(25);
  }
  else {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(20, 20, "GAME OVER!");
    u8g2.drawStr(10, 40, "Press OK to Restart");
    u8g2.sendBuffer();

    if (digitalRead(BTN_OK) == LOW) {
      delay(300);
      resetGame();
      beep(1000, 80);
    }
  }
}
