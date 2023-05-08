#include <elapsedMillis.h>

class Stepper {
public:
  Stepper(uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4, uint8_t sensor)
    : motor_pins{ in1, in2, in3, in4 }, sensor_pin{sensor} {
    for (const auto pin : motor_pins) {
      pinMode(pin, OUTPUT);
    }
    pinMode(sensor_pin, INPUT_PULLUP);
  }

  int step(int direction) {
    if (direction == 0 || last_step_us < min_interval_us) {
      return 0;
    }

    auto change = direction > 0 ? 1 : -1;
    sequence_no += change;
    sequence_no %= 8;
    uint8_t sequence[8]{
        0b1000,
        0b1001,
        0b0001,
        0b0011,
        0b0010,
        0b0110,
        0b0100,
        0b1100
      };
    for (uint8_t i = 0; i < 4; i++) {
      digitalWrite(motor_pins[i], sequence[sequence_no] & 1 << i);
    }
    last_step_us = 0;

    position_ += change;
    return change;
  }

  int32_t position() const { return position_; }
  int32_t target() const {return target_;}
  void set_target(int32_t target) {target_ = target;}

  enum class State {
    Init,
    Run,
    Idle
  };

  void loop() {
    switch (state_) {
      case State::Init:
      init_loop();
      break;
      case State::Run:
      run_loop();
      break;
      case State::Idle:
      idle_loop();
      break;
    }
  }

private:
  State state_ {State::Init};
  int32_t position_ {0};
  int32_t target_ {0};

  uint8_t motor_pins[4];
  uint8_t sensor_pin;

  elapsedMicros last_step_us{ 0 };
  uint16_t min_interval_us{ 1600 };

  uint8_t sequence_no{ 0 };

  void init_loop() {
    if(digitalRead(sensor_pin) == LOW) {
      position_ = 0;
      target_ = 0;
      Serial.println("State = Run");
      state_ = State::Run;
    } else {
      step(-1);
    }
  }

  void run_loop() {
    if(target_ == position_) {
      // for(uint8_t i = 0; i<4; ++i) {
      //   digitalWrite(motor_pins[i], 0);
      // }
      Serial.println("State = Idle");
      state_ = State::Idle;
    } else {
      step(target_ - position_);
    }
  }

  void idle_loop() {
    if(target_ != position_){
      Serial.println("State = Run");
      state_ = State::Run;
    }
  }
};