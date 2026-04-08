#include <CuteBuzzerSounds.h>
const int buzzer =  25;
void setup() {
  pinMode(13, OUTPUT);
    cute.init(buzzer);
    cute.play(S_CONNECTION);
}
void loop() {
    cute.play(S_SUPER_HAPPY);
    cute.play(S_HAPPY);
    cute.play(S_HAPPY_SHORT);
    cute.play(S_SAD);
    cute.play(S_CONFUSED);
    cute.play(S_CUDDLY);
    cute.play(S_OHOOH);
    cute.play(S_OHOOH2);
    cute.play(S_SURPRISE);
    cute.play(S_BUTTON_PUSHED);
    cute.play(S_FART1);
    cute.play(S_SLEEPING);
    cute.play(S_DISCONNECTION);
    delay(10000);
}