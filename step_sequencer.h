#pragma once

#include <arduino-timer.h>

#include "pins.h"

class TrainingStep {
public:
  float target_spm;
  unsigned long duration;
  const char *instruction;
public:
  TrainingStep(float target_spm, unsigned long duration, const char *instruction): target_spm(target_spm), duration(duration) {
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
  unsigned long _seq_start_millis;
  unsigned long _step_start_millis;
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
    _step_task = _timer.in(step->duration * 1000UL, _toNextStep, this);
  } 

  // This converts static calling to a method call
  static void _toNextStep(void *seq) {
    ((StepSequencer *)seq)->toNextStep();
  }
  
  void toNextStep() {
    tone(TONE, 440, 60);
    delay(60);
    tone(TONE, 880, 60);
    
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
    tone(TONE, 880, 60);
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

  bool running() {
    return _running;
  }

  unsigned long runtime() {
    return _running ? (millis() - _seq_start_millis) / 1000 : 0;
  }

  unsigned long steptime() {
    return _running ? (millis() - _step_start_millis) / 1000 : 0;
  }

  unsigned long steptimeLeft() {
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
