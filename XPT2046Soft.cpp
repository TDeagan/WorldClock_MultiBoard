#include "XPT2046Soft.h"

namespace {
static constexpr uint8_t COMMAND_X  = 0xD0;
static constexpr uint8_t COMMAND_Y  = 0x90;
static constexpr uint8_t COMMAND_Z1 = 0xB0;
static constexpr uint8_t COMMAND_Z2 = 0xC0;
}

void XPT2046Soft::begin(
  int pinClock,
  int pinMosi,
  int pinMiso,
  int pinChipSelect,
  int pinIrq,
  uint16_t pressureMinimum,
  uint8_t medianSamples
) {
  clockPin = pinClock;
  mosiPin = pinMosi;
  misoPin = pinMiso;
  chipSelectPin = pinChipSelect;
  irqPin = pinIrq;
  minimumPressure = pressureMinimum;

  if (medianSamples < 3) medianSamples = 3;
  if (medianSamples > 15) medianSamples = 15;
  if ((medianSamples & 1U) == 0U) ++medianSamples;
  sampleCount = medianSamples;

  pinMode(clockPin, OUTPUT);
  pinMode(mosiPin, OUTPUT);
  pinMode(misoPin, INPUT);
  pinMode(chipSelectPin, OUTPUT);
  if (irqPin >= 0) pinMode(irqPin, INPUT);

  digitalWrite(clockPin, LOW);
  digitalWrite(mosiPin, LOW);
  digitalWrite(chipSelectPin, HIGH);
}

bool XPT2046Soft::isPressed() const {
  if (irqPin < 0) return true;
  return digitalRead(irqPin) == LOW;
}

void XPT2046Soft::setPressureMinimum(uint16_t value) {
  minimumPressure = value;
}

uint16_t XPT2046Soft::getPressureMinimum() const {
  return minimumPressure;
}

uint8_t XPT2046Soft::transferByte(uint8_t outgoing) {
  uint8_t incoming = 0;

  for (uint8_t bit = 0; bit < 8; ++bit) {
    digitalWrite(mosiPin, (outgoing & 0x80U) ? HIGH : LOW);
    outgoing <<= 1;

    delayMicroseconds(1);
    digitalWrite(clockPin, HIGH);
    delayMicroseconds(1);

    incoming <<= 1;
    if (digitalRead(misoPin)) incoming |= 1U;

    digitalWrite(clockPin, LOW);
    delayMicroseconds(1);
  }

  return incoming;
}

uint16_t XPT2046Soft::read12(uint8_t command) {
  digitalWrite(chipSelectPin, LOW);
  delayMicroseconds(1);

  transferByte(command);
  const uint16_t high = transferByte(0x00);
  const uint16_t low = transferByte(0x00);

  digitalWrite(chipSelectPin, HIGH);
  delayMicroseconds(1);

  return static_cast<uint16_t>(((high << 8) | low) >> 3) & 0x0FFFU;
}

uint16_t XPT2046Soft::median(uint16_t *values, uint8_t count) {
  for (uint8_t i = 1; i < count; ++i) {
    const uint16_t value = values[i];
    int j = static_cast<int>(i) - 1;

    while (j >= 0 && values[j] > value) {
      values[j + 1] = values[j];
      --j;
    }

    values[j + 1] = value;
  }

  return values[count / 2];
}

bool XPT2046Soft::read(Xpt2046Sample &sample) {
  sample = Xpt2046Sample{};

  if (irqPin >= 0 && !isPressed()) return false;

  uint16_t xValues[15];
  uint16_t yValues[15];
  uint16_t pressureValues[15];
  uint8_t completed = 0;

  for (uint8_t i = 0; i < sampleCount; ++i) {
    if (irqPin >= 0 && !isPressed()) break;

    const uint16_t rawY = read12(COMMAND_Y);
    const uint16_t rawX = read12(COMMAND_X);
    const uint16_t z1 = read12(COMMAND_Z1);
    const uint16_t z2 = read12(COMMAND_Z2);
    const uint16_t pressure =
      static_cast<uint16_t>(z1 + (4095U - z2));

    yValues[completed] = rawY;
    xValues[completed] = rawX;
    pressureValues[completed] = pressure;
    ++completed;
  }

  if (completed < 3) return false;

  sample.rawX = median(xValues, completed);
  sample.rawY = median(yValues, completed);
  sample.pressure = median(pressureValues, completed);
  sample.contact = (irqPin >= 0) ? true : sample.pressure >= minimumPressure;
  sample.pressed = sample.contact && sample.pressure >= minimumPressure;
  return sample.contact;
}
