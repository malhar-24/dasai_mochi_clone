int buzzer = 2; // ESP32-C6 buzzer pin

void setup() {
  pinMode(buzzer, OUTPUT);
}

void loop() {
  // Cycle through the new, expressive emotions
  bumbleBee_LaughingJoy();
  delay(3000); 

  bumbleBee_SadDejected();
  delay(3000);

  bumbleBee_AngryFrustrated_Revised();
  delay(3000);
}

// ---------------------------
// Bumblebee Expressive Emotions Functions
// ---------------------------

// 😂 Laughing/Joy: Rapid, ascending, bouncy chirps with pauses, like short, cheerful bursts of air.
void bumbleBee_LaughingJoy() {
  int duration = 50; // Short duration for a bouncy, light feel
  
  // First laugh burst: ascending quickly
  for (int i = 0; i < 3; i++) {
    buzzSyllable(900 + (i * 100), duration);
    delay(20); // Very short gap
  }
  delay(150); // Pause between bursts

  // Second burst: slightly higher and faster
  for (int i = 0; i < 4; i++) {
    buzzSyllable(1100 + (i * 80), duration - 10);
    delay(15);
  }
  delay(200);

  // Final giggle: a quick, high flourish
  buzzSyllable(1400, 60);
  buzzSyllable(1300, 60);

  noTone(buzzer);
}

// 😢 Sad/Dejected: Slow, drawn-out, descending tones, emphasizing low frequency and long duration.
void bumbleBee_SadDejected() {
  // A long, low, wavering tone (sigh)
  tone(buzzer, 400, 400); // Very low, sustained tone
  delay(420);
  
  // A slowly descending phrase (gloom)
  for(int f = 800; f >= 500; f -= 50) {
    tone(buzzer, f, 120);
    delay(150); // Long delay makes it sound slow and mournful
  }

  // A final, soft, low chirp (teardrop)
  buzzSyllable(450, 80);

  noTone(buzzer);
}

// 😠 Angry/Frustrated (Revised): Staccato, mid-low tones, and a sudden, sharp sputter.
void bumbleBee_AngryFrustrated_Revised() {
  // Low, repetitive, short bursts (stomping foot)
  for (int i = 0; i < 5; i++) {
    buzzSyllable(600, 70);
    delay(40); 
  }
  delay(100);

  // A sudden, sharp spike followed by a sputter (outburst of rage)
  tone(buzzer, 1600, 50); // Quick, high spike (still short to avoid grating)
  delay(60); 
  
  // Grinding sputter
  for(int i=0; i<3; i++){
    tone(buzzer, 700 + (i % 2) * 50, 40);
    delay(45);
    noTone(buzzer); // Choppy sound for frustration
    delay(30);
  }

  noTone(buzzer);
}

// ---------------------------
// Helper function for single "syllable"
// ---------------------------
void buzzSyllable(int frequency, int duration){
  tone(buzzer, frequency, duration);
  delay(duration + 20); 
}