#pragma once

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

