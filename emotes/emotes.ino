//***********************************************************************************************
// ESP32 RoboEyes + CuteBuzzerSounds + Web Control
// NON-BLOCKING SOUND using FreeRTOS
//***********************************************************************************************

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>
#include <CuteBuzzerSounds.h>
#include <WiFi.h>
#include <WebServer.h>

// ---------------- OLED ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RoboEyes<Adafruit_SSD1306> roboEyes(display);

// ---------------- BUZZER ----------------
#define BUZZER_PIN 25   // ESP32 SAFE PIN

// ---------------- TIMING ----------------
unsigned long lastChange = 0;
const unsigned long emotionInterval = 40000;
byte step = 0;

// ---------------- FREERTOS ----------------
TaskHandle_t soundTaskHandle = NULL;
int soundToPlay = -1;

// ---------------- WIFI + WEBSERVER ----------------
WebServer server(80);

const char *ssid = "RoboEyes";
const char *password = "12345678";

// ---------- SOUND TASK (NON-BLOCKING) ----------
void soundTask(void *parameter) {
  while (true) {
    if (soundToPlay != -1) {
      cute.play(soundToPlay);   // BLOCKS ONLY THIS TASK
      soundToPlay = -1;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// ---------- WEB PAGE ----------
String htmlPage() {
  String page = "<html><body style='font-family:Arial;text-align:center;'>";
  page += "<h2>RoboEyes Step Controller</h2>";

  for (int i = 0; i < 15; i++) {
    page += "<a href='/set?step=" + String(i) + "'>";
    page += "<button style='padding:15px;margin:10px;font-size:20px;'>Step ";
    page += String(i);
    page += "</button></a><br>";
  }

  page += "<p>Current step: <b>" + String(step) + "</b></p>";
  page += "</body></html>";
  return page;
}

// ---------- WEB HANDLERS ----------
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleSet() {
  if (server.hasArg("step")) {
    step = server.arg("step").toInt();
    lastChange = millis()+ emotionInterval;  // apply instantly
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1);
  }

  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 60);
  roboEyes.setCuriosity(OFF);
  roboEyes.setAutoblinker(OFF, 0, 0);
  roboEyes.setIdleMode(OFF, 0, 0);

  roboEyes.setWidth(36, 36);
  roboEyes.setHeight(36, 36);
  roboEyes.setBorderradius(8, 8);
  roboEyes.setSpacebetween(12);

  pinMode(BUZZER_PIN, OUTPUT);
  cute.init(BUZZER_PIN);

  WiFi.softAP(ssid, password);
  Serial.println("WiFi Hotspot Started!");
  Serial.println("SSID: RoboEyes");
  Serial.println("IP:   192.168.4.1");

  // WebServer Routes
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.begin();

  // Create sound task on Core 0
  xTaskCreatePinnedToCore(
    soundTask,
    "SoundTask",
    2000,
    NULL,
    1,
    &soundTaskHandle,
    0
  );

  // Startup
  roboEyes.setMood(DEFAULT);
  roboEyes.setPosition(DEFAULT);
  soundToPlay = S_CONNECTION;
}

void blinkMultiple(uint8_t count, uint16_t blinkTime = 250) {
  for (uint8_t i = 0; i < count; i++) {
    roboEyes.blink();

    unsigned long t = millis();
    while (millis() - t < blinkTime) {
      roboEyes.update();
    }
  }
}

void loop() {
  server.handleClient();       // <<< IMPORTANT
  roboEyes.update();

  if (millis() - lastChange > emotionInterval) {
    lastChange = millis();
    step++;
    if(step == 1000) step = 0;

    roboEyes.setPosition(DEFAULT);
    unsigned long t = millis();


    switch (step % 15) {

      case 0: // laughing
        roboEyes.setMood(HAPPY);
        roboEyes.open();

        // Blink FIRST (visible)
        roboEyes.setVFlicker(ON,2);
        
        soundToPlay = S_SUPER_HAPPY;
        while (millis() - t < 200) {
          roboEyes.update();
        }
        // Play sound NON-BLOCKING
        blinkMultiple(3, 200);
        roboEyes.setVFlicker(OFF);
        roboEyes.setMood(DEFAULT);
        break;
      case 1: // HAPPY
        roboEyes.setMood(HAPPY);
        roboEyes.open();
        soundToPlay = S_HAPPY;

        // Blink FIRST (visible)
        roboEyes.setVFlicker(ON,2);
        
        while (millis() - t < 600) {
          roboEyes.update();
        }
        // Play sound NON-BLOCKING
        soundToPlay = S_HAPPY;
        
        roboEyes.setVFlicker(OFF);
        roboEyes.setMood(DEFAULT);
        break;
      case 2: // SUPER HAPPY
        roboEyes.setMood(HAPPY);
        roboEyes.open();
        soundToPlay = S_HAPPY_SHORT;

        // Blink FIRST (visible)
        roboEyes.setHFlicker(ON,2);
        
        while (millis() - t < 800) {
          roboEyes.update();
        }
        
        // Play sound NON-BLOCKING
        
        roboEyes.setHFlicker(OFF);
        roboEyes.setMood(DEFAULT);
        break;
      case 3: // sad
        roboEyes.setMood(TIRED);
        roboEyes.open();
        soundToPlay = S_SAD;
        roboEyes.setPosition(S);
        
        
        while (millis() - t < 300) {
          roboEyes.update();
        }
        blinkMultiple(3, 1000);
        roboEyes.setMood(DEFAULT);
        roboEyes.setPosition(DEFAULT);
        break;
      case 4: // confused
        roboEyes.anim_confused();
        soundToPlay = S_CONFUSED;
        while (millis() - t < 300) {
          roboEyes.update();
        }
        roboEyes.setMood(DEFAULT);
        roboEyes.setPosition(DEFAULT);
        break;
      case 5: // petting
        roboEyes.setMood(HAPPY);
        roboEyes.open();
        soundToPlay = S_CUDDLY;
        roboEyes.setPosition(N);
        roboEyes.setHFlicker(ON,2);
        while (millis() - t < 1500) {
          roboEyes.update();
        }
        roboEyes.setMood(DEFAULT);
        roboEyes.setHFlicker(OFF);
        roboEyes.setPosition(DEFAULT);
        break;
      case 6: // angry
        roboEyes.setMood(ANGRY);
        roboEyes.open();
        soundToPlay = S_OHOOH;
        while (millis() - t < 2000) {
          roboEyes.update();
        }
        roboEyes.setMood(DEFAULT);
        roboEyes.blink();
        roboEyes.setHFlicker(OFF);
        roboEyes.setPosition(DEFAULT);
        break;
      case 7: // super angry
        roboEyes.setMood(ANGRY);
        roboEyes.open();
        roboEyes.setVFlicker(ON,2);
        roboEyes.setPosition(S);
        soundToPlay = S_OHOOH2;
        while (millis() - t < 2000) {
          roboEyes.update();
        }
        roboEyes.setMood(DEFAULT);
        roboEyes.blink();
        roboEyes.setVFlicker(OFF);
        roboEyes.setPosition(DEFAULT);
        break;
      case 8: // grow eyes
        roboEyes.setWidth(36, 36);
        roboEyes.setHeight(45, 45);
        roboEyes.setBorderradius(12, 12);
        roboEyes.setSpacebetween(12);
        soundToPlay = S_SURPRISE;
        while (millis() - t < 2000) {
          roboEyes.update();
        }
        roboEyes.setWidth(36, 36);
        roboEyes.setHeight(36, 36);
        roboEyes.setBorderradius(8, 8);
        roboEyes.setSpacebetween(12);
        roboEyes.setMood(DEFAULT);
        roboEyes.setVFlicker(OFF);
        roboEyes.setPosition(DEFAULT);
        break;
      case 9: // SUPER mean
        roboEyes.close();
        soundToPlay = S_BUTTON_PUSHED;
        while (millis() - t < 2000) {
          roboEyes.update();
        }
        roboEyes.open();
        break;
      case 10: // look right left
        roboEyes.setMood(DEFAULT);
        roboEyes.open();
        roboEyes.setPosition(E);
        soundToPlay = S_SLEEPING;
        while (millis() - t < 600) {
          roboEyes.update();
        }
        t = millis();
        roboEyes.setMood(DEFAULT);
        roboEyes.open();
        roboEyes.setPosition(W);
        while (millis() - t < 1000) {
          roboEyes.update();
        }
        roboEyes.setMood(DEFAULT);
        roboEyes.setPosition(DEFAULT);
        break;
      case 11: // one eye blink
        roboEyes.setMood(DEFAULT);
        roboEyes.open();
        soundToPlay = S_HAPPY_SHORT;
        roboEyes.blink(0,1);
        while (millis() - t < 800) {
          roboEyes.update();
        }
        
        roboEyes.setMood(DEFAULT);
        break;
      case 12: //look left
        roboEyes.setMood(DEFAULT);
        roboEyes.open();
        soundToPlay = S_CONFUSED;
        roboEyes.setCuriosity(ON);
        roboEyes.setPosition(W);
        while (millis() - t < 800) {
          roboEyes.update();
        }
        
        roboEyes.setMood(DEFAULT);
        roboEyes.setPosition(DEFAULT);
        roboEyes.setCuriosity(OFF);
        break;
      case 13: //look right
        roboEyes.setMood(DEFAULT);
        roboEyes.open();
        soundToPlay = S_CONFUSED;
        roboEyes.setCuriosity(ON);
        roboEyes.setPosition(E);
        while (millis() - t < 800) {
          roboEyes.update();
        }
        
        roboEyes.setMood(DEFAULT);
        roboEyes.setPosition(DEFAULT);
        roboEyes.setCuriosity(OFF);
        break;
      case 14: {
        // ---- PREP ROBOEYES ----
        roboEyes.close();
        roboEyes.setPosition(S);
        roboEyes.setCuriosity(OFF);
        roboEyes.setAutoblinker(OFF,0,0);
        roboEyes.setIdleMode(OFF,0,0);
        while (millis() - t < 300) {
          roboEyes.update();
        }
        display.clearDisplay();
        display.display();


        int cx = 64;
        int cy = 58;     // moved slightly lower because half circle goes higher
        int r  = 42;

        // -------- PERFECT 180° ARC --------
        auto drawArc = [&]() {
          for (int a = -180; a <= 0; a++) {
            float rad = a * 0.0174533;
            int x1 = cx + cos(rad) * (r - 1);
            int y1 = cy + sin(rad) * (r - 1);
            int x2 = cx + cos(rad) * (r - 3);
            int y2 = cy + sin(rad) * (r - 3);
            display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
          }
        };

        // -------- MODERN TICK MARKS (EVERY 20 km/h) --------
        auto drawTicks = [&]() {
          for (int sp = 0; sp <= 300; sp += 20) {
            float angle = map(sp, 0, 300, -180, 0);
            float rad = angle * 0.0174533;

            // Tick line
            int x1 = cx + cos(rad) * (r - 2);
            int y1 = cy + sin(rad) * (r - 2);
            int x2 = cx + cos(rad) * (r - 12);
            int y2 = cy + sin(rad) * (r - 12);
            display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);

            // Number location
            int nx = cx + cos(rad) * (r - 20);
            int ny = cy + sin(rad) * (r - 20);

            display.setTextSize(1);
            display.setCursor(nx - 6, ny - 3);
            display.print(sp);
          }
        };

        // -------- NEEDLE (follows 180° arc) --------
        auto drawNeedle = [&](int speed) {
          float angle = map(speed, 0, 300, -180, 0);
          float rad = angle * 0.0174533;

          int nx = cx + cos(rad) * (r - 8);
          int ny = cy + sin(rad) * (r - 8);

          display.drawLine(cx, cy, nx, ny, SSD1306_WHITE);
          display.fillCircle(cx, cy, 3, SSD1306_WHITE);
        };

        // -------- SWEEP UP --------
        
        for (int speed = 0; speed <= 300; speed += 5) {
          display.clearDisplay();
          drawArc();
          drawTicks();
          drawNeedle(speed);

          // Big digital speed at bottom
          display.setTextSize(2);
          display.setCursor(64 - 18, 56);
          display.print(speed);

          display.display();
          delay(1);
          if(speed==10){
            soundToPlay = S_SURPRISE;
          }
        }

        // -------- SWEEP DOWN FAST --------
        for (int speed = 300; speed >= 0; speed -= 40) {
          display.clearDisplay();
          drawArc();
          drawTicks();
          drawNeedle(speed);

          display.setTextSize(2);
          display.setCursor(64 - 18, 56);
          display.print(speed);

          display.display();
          delay(20);
        }

        // CLEAR + RESTORE EYES
        display.clearDisplay();
        display.display();
        delay(50);

        roboEyes.setMood(DEFAULT);
        roboEyes.setPosition(DEFAULT);
        roboEyes.open();
        break;
    }










    }
  }
}
