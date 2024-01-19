#include <LiquidCrystal.h>
#include <Encoder.h>
#include <arduino-timer.h>

const int LED_1 = 8;
//const int LED_2 = 9;
const int TONE = 9;
const int IN_1 = 2;
const int IN_2 = 3;

LiquidCrystal lcd(12, 11, 5, 4, 7, 6);
Encoder enc(IN_1, IN_2);
Timer<1> display_timer;

class TrainingStep {
public:
  float target_spm;
  unsigned int duration;
  const char *instruction;
public:
  TrainingStep(float target_spm, unsigned int duration, const char *instruction): target_spm(target_spm), duration(duration) {
    this->instruction = instruction;
  }
};

#define STEPS_MAX 16
class StepSequencer {
private:
  TrainingStep *_training_steps[STEPS_MAX];
  int _add_idx;
  int _read_idx;
  Timer<10> _timer;
  Timer<>::Task _signal_task, _step_task;
  unsigned int _seq_start_millis;
  unsigned int _step_start_millis;
  bool _running;

  void _setToStep(int new_read_idx) {
    _step_start_millis = millis();
    _read_idx = new_read_idx;
    auto *step = currentStep();
    auto targ_spm = step->target_spm;
    if (targ_spm != 0) {
      _signal_task = _timer.every(60000.0 / step->target_spm, signalStart, &_timer);
    }
    // Schedule next step to start after current one is complete (step->duration seconds).
    _step_task = _timer.in(step->duration * 1000, _toNextStep, this);
  }

  // This converts static calling to a method call
  static void _toNextStep(void *seq) {
    ((StepSequencer *)seq)->toNextStep();
  }
  
  void toNextStep() {
    _timer.cancel(_signal_task);
    _timer.cancel(_step_task);

    if (_read_idx + 1 < _add_idx) {
      _setToStep(_read_idx + 1);
    } else {
      stop();
    }
  }
  
  static bool signalStart(Timer<> *t) {
    digitalWrite(LED_1, 1);
    tone(TONE, 440, 60);
    t->in(200, signalStop);
    return true; // keep timer active? true
  }
  
  static bool signalStop(void *) {
    digitalWrite(LED_1, 0);
    return true;
  }

public:
  StepSequencer(): _add_idx(0), _read_idx(0), _running(false) {}

  void addStep(TrainingStep &step) {
    _training_steps[_add_idx] = &step;
    ++_add_idx;
  }
  
  TrainingStep *currentStep() {
    return _training_steps[_read_idx];
  }
  
  unsigned int stepIdx() {
    return _read_idx;
  }

  unsigned int stepCount() {
    return _add_idx;
  }

  unsigned int runtime() {
    return _running ? (millis() - _seq_start_millis) / 1000 : 0;
  }

  bool running() {
    return _running;
  }

  unsigned int steptime() {
    return _running ? (millis() - _step_start_millis) / 1000 : 0;
  }

  unsigned int steptimeLeft() {
    return currentStep()->duration - steptime();
  }
  
  void start() {
    _running = true;
    _seq_start_millis = millis();
    
    _setToStep(0);
  }

  void stop() {
    _timer.cancel(_signal_task);
    _timer.cancel(_step_task);

    _running = false;
  }
  
  void tick() {
    _timer.tick();
  }
};

class Rower {
private:
  int _position;
  int _position_l;
  int _position_l2;
  int _position_begin;
  unsigned long _last_end;
  unsigned long _last_begin;

  unsigned int _strokes;
  unsigned int _stroke_length;

  float _spm_pull;
  float _spm_give;
  float _spm;

  StepSequencer *_seq;
public:
  Rower(StepSequencer *seq): // StepSequencer is needed because Rower starts the sequence
    _position(0), _position_l(0), _position_l2(0), _last_end(0), _last_begin(0), _strokes(0), _stroke_length(0), _spm_pull(0), _spm_give(0), _spm(0), _seq(seq) {};

  bool updatePosition(int new_position) {
    unsigned long now = millis();
    _position = new_position;
    if (_position != _position_l) {
      int diff = 0;

      // END reached
      if (_position < _position_l && _position_l2 < _position_l) {
        _stroke_length = _position - _position_begin +2; // not clear why?
        ++_strokes;

        // Timing
        diff = now - _last_end;
        _spm_pull = 60000.0f / diff;

        if (!_seq->running()) {
          _seq->start();
        }
      
        _last_end = now;
      }

      // BEGIN reached
      if (_position > _position_l && _position_l2 > _position_l) {
        _position_begin = _position;
      
        // Timing
        diff = now - _last_begin;
        _spm_give = 60000.0f / diff;
      
        _last_begin = now;
      }

      _position_l2 = _position_l;
      _position_l = _position;
      
      return true; // Position updated
    }

    return false;
  }

  float spm() {
    return (_spm_pull + _spm_give) / 2.0;
  }

  int position() { return _position; }
  unsigned int strokes() { return _strokes; }
  unsigned int stroke_length() { return _stroke_length; }
};

auto seq = StepSequencer();
auto rower = Rower(&seq);

TrainingStep training_steps[] = {
   TrainingStep(0, 2*60, "WrmUp"),
   TrainingStep(20, 2*60, "Row"),
   TrainingStep(0, 1*60, "Arm&Bod"),
   TrainingStep(22, 2*60, "Row"),
   TrainingStep(0, 1*60, "Leg&Hip"),
   TrainingStep(20, 2*60, "Row"),
   TrainingStep(0, 1*60, "Arms"),
   TrainingStep(18, 2*60, "Row"),
   TrainingStep(0, 1*60, "St&Pa"),
   TrainingStep(18, 2*60, "Row"),
   TrainingStep(0, 1*60, "St&PaAw"),
   TrainingStep(22, 2*60, "Row")
};

void setup() {
    lcd.begin(16, 2);
    lcd.clear();
  
    pinMode(IN_1, INPUT_PULLUP);
    pinMode(IN_2, INPUT_PULLUP);
  
    pinMode(LED_1, OUTPUT);
    pinMode(TONE, OUTPUT);

    for (int i = 0; i < 12; ++i) {
      seq.addStep(training_steps[i]);
    }

    tone(TONE, 440, 20);
    delay(20);
    tone(TONE, 680, 20);
    delay(20);
    tone(TONE, 300, 80);

    display_timer.every(1000, printStateGlob);
    printState(seq, rower);
}

void printStateGlob(void *) {
  printState(seq, rower);
}

void printState(StepSequencer &seq, Rower &rower) {
  lcd.clear();
  
  lcd.setCursor(0, 0);
  lcd.print(String("M") + String(rower.spm()));

  lcd.setCursor(7, 0);
  lcd.print(String(seq.steptimeLeft()));

  lcd.setCursor(12, 0);
  lcd.print(String(seq.stepIdx() + 1) + String("/") + String(seq.stepCount()));

  lcd.setCursor(0, 1);
  lcd.print(String("T") + String(seq.currentStep()->target_spm));

  lcd.setCursor(9, 1);
  lcd.print(String(seq.currentStep()->instruction));
}

void loop() {
  seq.tick();
  display_timer.tick(); // Updates display every second
  
  auto position = enc.read() / 2;
  if (rower.updatePosition(position)) { 
      printState(seq, rower);
  }
}
