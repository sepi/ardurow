#include <LiquidCrystal.h>
#include <Encoder.h>
#include <arduino-timer.h>

#include "pins.h"
#include "step_sequencer.h"
#include "rower.h"

LiquidCrystal lcd(12, 11, 5, 4, 7, 6);
Encoder enc(IN_1, IN_2);
Timer<1> display_timer;

auto seq = StepSequencer();
auto rower = Rower(&seq);

const unsigned short STEP_COUNT = 14;
TrainingStep training_steps[] = {
   TrainingStep(0, 2*60UL, "WrmUp"),
   TrainingStep(20, 2*60UL, "Row"),
   TrainingStep(0, 1*60UL, "Arm&Bod"),
   TrainingStep(22, 2*60UL, "Row"),
   TrainingStep(0, 1*60UL, "Leg&Hip"),
   TrainingStep(20, 2*60UL, "Row"),
   TrainingStep(0, 1*60UL, "Arms"),
   TrainingStep(18, 2*60UL, "Row"),
   TrainingStep(0, 1*60UL, "St&Pa"),
   TrainingStep(18, 2*60UL, "Row"),
   TrainingStep(0, 1*60UL, "St&PaAw"),
   TrainingStep(20, 2*60UL, "Row"),
   TrainingStep(0, 1*60UL, "St&PaAw"),
   TrainingStep(22, 2*60UL, "Row")
};

void printStateGlob(void *) {
  printState(seq, rower);
}

void printState(StepSequencer &seq, Rower &rower) {
  lcd.clear();
  
  lcd.setCursor(0, 0);
  lcd.print(String("M") + String(rower.spm()));

  lcd.setCursor(7, 0);
  lcd.print(String(seq.steptimeLeft()));

  lcd.setCursor(11, 0);
  lcd.print(String(seq.stepIdx() + 1) + String("/") + String(seq.stepCount()));

  lcd.setCursor(0, 1);
  lcd.print(String("T") + String(seq.currentStep()->target_spm) + String("s"));

  lcd.setCursor(7, 1);
  lcd.print(String(seq.currentStep()->instruction));
}

void setup() {
    lcd.begin(16, 2);
    lcd.clear();
  
    pinMode(IN_1, INPUT_PULLUP);
    pinMode(IN_2, INPUT_PULLUP);
  
    pinMode(LED_1, OUTPUT);
    pinMode(TONE, OUTPUT);

    for (int i = 0; i < STEP_COUNT; ++i) {
      seq.addStep(training_steps[i]);
    }

    lcd.setCursor(0, 0);
    lcd.print("  Welcome to    ");
    lcd.setCursor(0, 1);
    lcd.print("    ArduRow     ");

    tone(TONE, 880, 20);
    delay(20);
    tone(TONE, 440, 20);
    delay(20);
    tone(TONE, 300, 80);

    delay(3000); // For welcome

    display_timer.every(333, printStateGlob);
    printState(seq, rower);
}

void loop() {
  seq.tick();
  display_timer.tick(); // Updates display
  
  auto position = enc.read() / 2;
  if (rower.updatePosition(position)) { 
      printState(seq, rower);
  }
}
