/* 
  ESP32 WROOM - Full combined sketch
  Dino Game + Timer + Pomodoro
  Web-controlled via ESP32 SoftAP (Hotspot)
  Buzzer on GPIO 25
  Uses WiFi.softAP + WebServer (synchronous) to avoid ESPAsyncWebServer mbedtls issues
*/

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>

// ---------- HARDWARE ----------
#define BUZZER 25

// ---------- DISPLAY ----------
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ---------- AP (Hotspot) ----------
const char* ap_ssid     = "ESP32_3_in_1_Controller";
const char* ap_password = "password12345";

// ---------- Web server ----------
WebServer server(80);

// ---------- MODE ----------
enum Mode { MODE_TIMER=0, MODE_DINO=1, MODE_POMODORO=2 ,MODE_HAPPYSCORE=3 };
volatile Mode currentMode = MODE_TIMER;

// ---------- Button flags (set by web) ----------
volatile bool webInc = false;    // maps to BTN_INC / INC
volatile bool webOk  = false;    // maps to BTN_OK / OK
volatile bool webJump = false;   // maps to BTN_JUMP (dino)
volatile bool webRestart = false; // restart for dino

// ---------- COMMON helper ----------
void beep(int f=1500, int d=200) {
  tone(BUZZER, f, d);
}

// ============================================================================
//                                 TIMER MODE
// ============================================================================
namespace TimerMode {
  int d[4] = {0,0,0,0};
  int pos = 0;
  bool timerRunning = false;
  unsigned long lastTick = 0;

  void beepFinish() {
    for (int i=0;i<4;i++){
      beep(1500 + (i%2)*300, 120);
      delay(120);
    }
  }

  void drawTimer() {
    char buf[6];
    sprintf(buf, "%d%d:%d%d", d[0], d[1], d[2], d[3]);

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_logisoso22_tf);

    int w = u8g2.getStrWidth(buf);
    int xs = (128 - w) / 2;
    int y = 42;
    u8g2.drawStr(xs, y, buf);

    int wDigit = u8g2.getStrWidth("8");
    int wColon = u8g2.getStrWidth(":");

    int x0 = xs;
    int x1 = x0 + wDigit;
    int x2 = x1 + wDigit + wColon;
    int x3 = x2 + wDigit;
    int digitX[4] = { x0, x1, x2, x3 };

    if (!timerRunning) {
      u8g2.drawBox(digitX[pos], y + 4, wDigit, 3);
    }

    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.sendBuffer();
  }

  void decreaseTimer() {
    if (millis() - lastTick < 1000) return;
    lastTick = millis();

    int totalSec = d[0]*600 + d[1]*60 + d[2]*10 + d[3];
    if (totalSec <= 0) {
      timerRunning = false;
      beepFinish();
      return;
    }
    totalSec--;
    d[0] = totalSec / 600; totalSec %= 600;
    d[1] = totalSec / 60;  totalSec %= 60;
    d[2] = totalSec / 10;
    d[3] = totalSec % 10;
  }

  void setup() {
    d[0]=d[1]=d[2]=d[3]=0;
    pos = 0;
    timerRunning = false;
    lastTick = millis();
    drawTimer();
  }

  void loop() {
    if (!timerRunning) {
      if (webInc) {
        d[pos]++;
        if (d[pos] > 9) d[pos] = 0;
      }
      if (webOk) {
        pos++;
        if (pos > 3) {
          pos = 0;
          timerRunning = true;
          lastTick = millis();
        }
      }
    } else {
      decreaseTimer();
    }
    drawTimer();
    // small delay to keep UI smooth but responsive to web
    delay(50);
  }
}

// ============================================================================
//                                 DINO MODE
// ============================================================================
namespace DinoMode {
  const int SCALE = 2;
  const int groundY = 63;
  const int jumpPeak = groundY - 65;
  const int dinoOrigW = 15;
  const int dinoOrigH = 17;
  const int dinoW = dinoOrigW * SCALE;
  const int dinoH = dinoOrigH * SCALE;

  int dinoY = groundY;
  int velocity = 0;
  bool inAir = false;

  // dino bitmap
  const uint8_t dinoBitmap[17][15] PROGMEM = {
    {0,0,0,0,0,0,0,0,1,1,1,1,1,0,0},{0,0,0,0,0,0,0,1,1,1,1,1,1,1,1},
    {0,0,0,0,0,0,0,1,1,0,1,1,1,1,1},{0,0,0,0,0,0,0,1,1,1,1,1,1,1,1},
    {0,0,0,0,0,0,0,1,1,1,1,1,0,0,0},{0,0,0,0,0,0,0,1,1,1,1,1,1,1,0},
    {1,1,0,0,0,0,1,1,1,1,0,0,0,0,0},{1,0,0,0,0,1,1,1,1,1,0,0,0,0,0},
    {1,0,0,1,1,1,1,1,1,1,1,1,1,0,0},{1,0,1,1,1,1,1,1,1,1,0,1,1,0,0},
    {1,1,1,1,1,1,1,1,1,1,0,0,0,0,0},{1,1,1,1,1,1,1,1,1,1,0,0,0,0,0},
    {0,1,1,1,1,1,1,1,1,0,0,0,0,0,0},{0,0,1,1,1,1,1,1,0,0,0,0,0,0,0},
    {0,0,0,1,1,0,1,1,0,0,0,0,0,0,0},{0,0,0,1,0,0,1,0,0,0,0,0,0,0,0},
    {0,0,0,1,1,0,1,1,0,0,0,0,0,0,0}
  };

  // cactus shapes in compact arrays (use same dims from earlier)
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

  struct CactusType { const uint8_t *bitmap; int h,w; };
  CactusType cactusShapes[3] = {
    {(const uint8_t*)cactus1, 15, 10},
    {(const uint8_t*)cactus2, 11, 10},
    {(const uint8_t*)cactus3, 12, 7}
  };

  int cactusIndex = 0;
  int obsX = 128;
  bool gameOver = false;

  void drawScaledPixel(int x, int y) {
    for (int dx=0; dx<SCALE; dx++)
      for (int dy=0; dy<SCALE; dy++)
        u8g2.drawPixel(x+dx, y+dy);
  }

  void drawDino() {
    for (int y=0;y<dinoOrigH;y++){
      for (int x=0;x<dinoOrigW;x++){
        if (pgm_read_byte(&dinoBitmap[y][x])){
          drawScaledPixel(10 + x*SCALE, (dinoY - dinoH) + y*SCALE);
        }
      }
    }
  }

  void drawCactus() {
    const CactusType &C = cactusShapes[cactusIndex];
    for (int y=0;y<C.h;y++){
      for (int x=0;x<C.w;x++){
        uint8_t px = pgm_read_byte((const uint8_t*)C.bitmap + y*C.w + x);
        if (px) drawScaledPixel(obsX + x*SCALE, (groundY - C.h*SCALE) + y*SCALE);
      }
    }
  }

  void drawGround() {
    u8g2.drawHLine(0, groundY, 128);
  }

  bool checkCollision() {
    int dinoX1 = 10, dinoX2 = dinoX1 + dinoW;
    int dinoY1 = dinoY - dinoH, dinoY2 = dinoY;
    const CactusType &C = cactusShapes[cactusIndex];
    int obsX1 = obsX, obsX2 = obsX + C.w*SCALE;
    int obsY1 = groundY - C.h*SCALE, obsY2 = groundY;
    return !(dinoX2 < obsX1 || dinoX1 > obsX2 || dinoY2 < obsY1 || dinoY1 > obsY2);
  }

  void resetGame() {
    dinoY = groundY;
    velocity = 0;
    inAir = false;
    obsX = 128;
    cactusIndex = random(0,3);
    gameOver = false;
  }

  void setup() {
    randomSeed(millis());
    resetGame();
  }

  void loop() {
    if (!gameOver) {
      if (!inAir && webJump) {
        velocity = -11;
        inAir = true;
        beep(1500, 40);
      }

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

      obsX -= 4;
      if (obsX < -50) { obsX = 128; cactusIndex = random(0,3); }

      if (checkCollision()) {
        beep(200, 300);
        gameOver = true;
      }

      u8g2.clearBuffer();
      drawGround();
      drawDino();
      drawCactus();
      u8g2.setFont(u8g2_font_6x12_tr);
      u8g2.sendBuffer();

      delay(25);
    } else {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_6x12_tr);
      u8g2.drawStr(20, 20, "GAME OVER!");
      u8g2.drawStr(10, 40, "Press OK to Restart");
      u8g2.sendBuffer();

      if (webOk) {
        resetGame();
        beep(1000, 80);
      }
      delay(50);
    }
  }
}

// ============================================================================
//                               POMODORO MODE
// ============================================================================
namespace PomodoroMode {
  bool isWork = true;
  unsigned long periodSec = 1*60; // default work period 20 min
  unsigned long startTime = 0;
  #define PIXELS_PER_MIN 12

  void startPeriod(){
    if (isWork) periodSec = 20 * 60; // work
    else periodSec = 10 * 60;         // short break
    startTime = millis();
  }

  void beepTone() {
    for (int i=0;i<4;i++){
      beep(1500 + (i%2)*300, 120);
      delay(120);
    }
  }

  void drawUI() {
    u8g2.clearBuffer();
    unsigned long elapsed = (millis() - startTime) / 1000;
    if (elapsed > periodSec) elapsed = periodSec;
    unsigned long remain = periodSec - elapsed;

    int centerX = 64;
    u8g2.drawLine(centerX, 15, centerX, 33);

    int scaleY_bottom = 28;
    int scaleY_top = 23;
    int numberY = 40;

    long totalPixels = (periodSec * PIXELS_PER_MIN) / 60;
    long shift = (elapsed * totalPixels) / periodSec;

    for (int m=0; m <= (periodSec / 60); m++) {
      long x = centerX + (m * PIXELS_PER_MIN) - shift;
      if (x < -5 || x > 133) continue;
      u8g2.drawLine(x, scaleY_bottom, x, scaleY_top);
      char buf[4]; sprintf(buf, "%d", m);
      u8g2.setFont(u8g2_font_tom_thumb_4x6_tr);
      u8g2.drawStr(x - 2, numberY, buf);
    }

    for (int s = 0; s <= periodSec; s += 10) {
      long x = centerX + (s * PIXELS_PER_MIN) / 60 - shift;
      if (x < -5 || x > 133) continue;
      u8g2.drawPixel(x, scaleY_bottom - 1);
    }

    char timeBuf[10];
    sprintf(timeBuf, "%02d:%02d", (int)(remain/60), (int)(remain%60));
    u8g2.setFont(u8g2_font_logisoso16_tf);
    u8g2.drawStr(35, 60, timeBuf);

    u8g2.setFont(u8g2_font_6x12_tr);
    if (isWork) u8g2.drawStr(2, 10, "WORK"); else u8g2.drawStr(90, 10, "BREAK");
    u8g2.sendBuffer();
  }

  void setup() {
    startTime = millis();
  }

  void loop() {
    drawUI();
    unsigned long elapsed = (millis() - startTime) / 1000;
    if (elapsed >= periodSec) {
      beepTone();
      isWork = !isWork;
      startPeriod();
    }
    delay(200);
  }
}

namespace HappyscoreMode {

  // -------- TARGET HAPPY SCORE --------
  uint8_t happyPercent = 100;   // target (0–100)

  // -------- ANIMATION STATE --------
  uint8_t displayPercent = 0; // animated value
  unsigned long lastAnimTick = 0;
  const uint16_t animInterval = 30; // ms per step

  void beepTone() {
    for (int i = 0; i < 4; i++) {
      beep(1500 + (i % 2) * 300, 120);
      delay(120);
    }
  }

  void drawUI(uint8_t percent) {
    u8g2.clearBuffer();

    // -------- TITLE --------
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(20, 12, "HAPPINESS SCORE");

    // -------- STATUS BAR --------
    int barX = 14;
    int barY = 26;
    int barW = 100;
    int barH = 10;

    u8g2.drawFrame(barX, barY, barW, barH);

    int fillW = (percent * (barW - 2)) / 100;
    if (fillW > 0) {
      u8g2.drawBox(barX + 1, barY + 1, fillW, barH - 2);
    }

    // -------- PERCENT TEXT --------
    char buf[6];
    sprintf(buf, "%d%%", percent);

    u8g2.setFont(u8g2_font_logisoso16_tf);
    int w = u8g2.getStrWidth(buf);
    u8g2.drawStr((128 - w) / 2, 58, buf);

    u8g2.sendBuffer();
  }

  void setup() {
    displayPercent = 0;        // reset animation
    lastAnimTick = millis();
  }

  void loop() {
    unsigned long now = millis();

    // Animate up to target
    if (displayPercent < happyPercent) {
      if (now - lastAnimTick >= animInterval) {
        displayPercent++;
        lastAnimTick = now;
      }
    }

    drawUI(displayPercent);
    delay(20);
  }
}


// ============================================================================
//                                 HTML PAGE
// ============================================================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 3-in-1 Controller</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, Helvetica, sans-serif; text-align:center; padding:10px; }
    h2 { margin:8px 0; }
    button { padding:12px 18px; margin:6px; font-size:16px; min-width:120px; }
    .controls { margin-top:12px; }
  </style>
</head>
<body>
  <h2>ESP32 — Dino / Timer / Pomodoro</h2>
  <div>
    <button onclick="fetch('/mode?set=timer')">TIMER</button>
    <button onclick="fetch('/mode?set=dino')">DINO</button>
    <button onclick="fetch('/mode?set=pomodoro')">POMODORO</button>
    <button onclick="fetch('/mode?set=happyscore')">happyscore</button>
  </div>
  <div class="controls" id="controls">
    <!-- controls will be visible per mode -->
  </div>

<script>
  function send(cmd){
    fetch('/action?cmd=' + cmd);
  }

  function updateControls(mode){
    const c = document.getElementById('controls');
    if(mode === 'timer'){
      c.innerHTML = `
        <button onclick="send('inc')">INCREASE</button>
        <button onclick="send('next')">NEXT</button>
      `;
    } else if(mode === 'dino'){
      c.innerHTML = `
        <button onclick="send('jump')">JUMP</button>
        <button onclick="send('ok')">OK</button>
      `;
    } else if(mode === 'pomodoro'){
      c.innerHTML = `
        <button onclick="send('pom_start')">START</button>
        <button onclick="send('pom_stop')">STOP</button>
      `;
    }
  }

  // Attach mode buttons
  document.querySelectorAll('button').forEach(btn=>{
    btn.addEventListener('click', ()=>{
      const mode = btn.innerText.toLowerCase();
      fetch('/mode?set=' + mode).then(()=>updateControls(mode));
    });
  });

  // initial controls
  window.onload = function(){ updateControls('timer'); }
</script>


</body>
</html>
)rawliteral";

// ============================================================================
//                              SERVER HANDLERS
// ============================================================================
void handleRoot() {
  server.send_P(200, "text/html", index_html);
}

void handleMode() {
  if (!server.hasArg("set")) {
    server.send(400, "text/plain", "Missing 'set' parameter");
    return;
  }
  String m = server.arg("set");
  if (m == "timer") {
    currentMode = MODE_TIMER;
    TimerMode::setup();
  } else if (m == "dino") {
    currentMode = MODE_DINO;
    DinoMode::setup();
  } else if (m == "pomodoro") {
    currentMode = MODE_POMODORO;
    PomodoroMode::setup();
  } else if (m == "happyscore") {
    currentMode = MODE_HAPPYSCORE;
    HappyscoreMode::setup();
  }
  server.send(200, "text/plain", "Mode set: " + m);
}

void handleAction() {
  if (!server.hasArg("cmd")) {
    server.send(400, "text/plain", "Missing 'cmd' parameter");
    return;
  }
  String cmd = server.arg("cmd");

  // Clear previous transient flags before setting new
  webInc = false; webOk = false; webJump = false; webRestart = false;

  if (cmd == "inc") {
    webInc = true;
  } else if (cmd == "next") {
    webOk = true;
  } else if (cmd == "jump") {
    webJump = true;
  } else if (cmd == "ok") {
    webOk = true;
  } else if (cmd == "pom_start") {
    // start pomodoro by switching mode then letting PomodoroMode loop handle restart
    currentMode = MODE_POMODORO;
    PomodoroMode::setup();
  } else if (cmd == "pom_stop") {
    // noop: Pomodoro runs automatically; user can toggle by re-setting mode to timer or resetting
  }

  server.send(200, "text/plain", "cmd:" + cmd);
}

// ============================================================================
//                                  SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(10);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  u8g2.begin();

  Serial.println();
  Serial.println("[*] Starting Soft AP...");
  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(ap_ssid, ap_password);
  if (!ok) {
    Serial.println("[!] softAP failed");
  } else {
    IPAddress ip = WiFi.softAPIP();
    Serial.print("[+] AP started at: ");
    Serial.println(ip);
  }

  // server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/mode", HTTP_GET, handleMode);
  server.on("/action", HTTP_GET, handleAction);
  server.begin();
  Serial.println("[+] Webserver started");

  // initial mode
  currentMode = MODE_TIMER;
  TimerMode::setup();
}

// ============================================================================
//                                   LOOP
// ============================================================================
void loop() {
  // handle HTTP (synchronous server)
  server.handleClient();

  // dispatch active mode loop
  if (currentMode == MODE_TIMER) {
    TimerMode::loop();
  } else if (currentMode == MODE_DINO) {
    DinoMode::loop();
  } else if (currentMode == MODE_POMODORO) {
    PomodoroMode::loop();
  } else if (currentMode == MODE_HAPPYSCORE) {
    HappyscoreMode::loop();
  }

  // clear transient web flags (simulate momentary click)
  webInc = false;
  webOk = false;
  webJump = false;
  webRestart = false;
}
