#pragma once

#include <Arduino.h>

struct Xpt2046Sample {
  uint16_t rawX = 0;
  uint16_t rawY = 0;
  uint16_t pressure = 0;
  bool contact = false;
  bool pressed = false;
};

class XPT2046Soft {
public:
  void begin(
    int pinClock,
    int pinMosi,
    int pinMiso,
    int pinChipSelect,
    int pinIrq,
    uint16_t pressureMinimum,
    uint8_t medianSamples
  );

  bool isPressed() const;
  bool read(Xpt2046Sample &sample);

  void setPressureMinimum(uint16_t value);
  uint16_t getPressureMinimum() const;

private:
  int clockPin = -1;
  int mosiPin = -1;
  int misoPin = -1;
  int chipSelectPin = -1;
  int irqPin = -1;
  uint16_t minimumPressure = 80;
  uint8_t sampleCount = 9;

  uint8_t transferByte(uint8_t outgoing);
  uint16_t read12(uint8_t command);
  static uint16_t median(uint16_t *values, uint8_t count);
};
