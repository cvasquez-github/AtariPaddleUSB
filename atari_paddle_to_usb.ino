#include <Joystick.h>
Joystick_ Joystick;

const unsigned long gcAnalogDelta = 10;
unsigned long gNextTime = 0;

int button1State = 0;
int button2State = 0;
int lastButton1State = 0;
int lastButton2State = 0;

void ResetAxes() {
  Joystick.setXAxis(0);
  Joystick.setYAxis(0);
}

int readA(uint8_t ch) {
  ADMUX = (ADMUX & 0x3F) | (1 << REFS0);
  ADMUX = (ADMUX & 0xF0) | (ch & 0x0F);
  analogRead(ch);
  delayMicroseconds(150);
  return analogRead(ch);
}

int readMedian3(uint8_t ch) {
  int a = readA(ch);
  int b = readA(ch);
  int c = readA(ch);
  if ((a <= b && b <= c) || (c <= b && b <= a)) return b;
  if ((b <= a && a <= c) || (c <= a && a <= b)) return a;
  return c;
}

int filteredA0 = 0;
int filteredA1 = 0;
const uint8_t ALPHA_NUM = 1;
const uint8_t ALPHA_DEN = 8;

int filter(int &filtered, int value) {
  filtered = filtered + ((value - filtered) * ALPHA_NUM) / ALPHA_DEN;
  return filtered;
}

void setup() {
  Serial.begin(9600);
  delay(50);

  Joystick.begin(true);
  analogReference(DEFAULT);
  ADMUX = (ADMUX & 0x3F) | (1 << REFS0);  // REFS=01
  DIDR0 |= (1 << ADC0D) | (1 << ADC1D);   // A0, A1

  readA(0);
  readA(1);

  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), handleButton1Change, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), handleButton2Change, CHANGE);
}

void handleButton1Change() {
  evaluateInput(2, button1State, lastButton1State, 1);
}

void handleButton2Change() {
  evaluateInput(3, button2State, lastButton2State, 2);
}

void evaluateInput(int input, int &currentValue, int &lastValue, int button) {
  currentValue = digitalRead(input);
  if (currentValue != lastValue) {
    if (currentValue == HIGH) {
      Joystick.setButton(button, 0);
    } else if (currentValue == LOW) {
      Joystick.setButton(button, 1);
    }
    lastValue = currentValue;
  }
}

void loop() {
  if (millis() >= gNextTime) {
    filteredA0 = filter(filteredA0, readMedian3(0));
    Joystick.setXAxis(map(filteredA0, 560, 1010, 0, 127));

    filteredA1 = filter(filteredA1, readMedian3(1));
    Joystick.setYAxis(map(filteredA1, 560, 1010, 0, 127));

    gNextTime = millis() + gcAnalogDelta;
    delay(1);
  }
}
