#include <Arduino.h>
#include <Wire.h>
#define X_PIN PIN_A6
#define Y_PIN PIN_A7

void receive(int len);
void request();
uint8_t buf[5];

void setup()
{
  pinMode(X_PIN, INPUT);
  pinMode(Y_PIN, INPUT);
  Wire.begin(0x0A);
  Wire.onReceive(receive);
  Wire.onRequest(request);
}

void receive(int len)
{
  while (Wire.available())
  {
    Wire.read();
  }
}

void request()
{
  Wire.write(buf, 5);
}

const int deadzone = 3;
const int shift = -8;
const int m = 4;

void loop()
{
  buf[0] = 0;
  buf[1] = 0;
  buf[2] = 0;
  buf[3] = 0;
  int a1 = ((analogRead(Y_PIN) - 256) / 32) + shift;
  int a2 = ((analogRead(X_PIN) - 256) / 32) + shift;
  if (a1 < -deadzone)
  {
    buf[0] = (-a1 - deadzone) / m;
  }
  else if (deadzone <= a1)
  {
    buf[1] = (a1 - deadzone) / m;
  }
  if (a2 < -deadzone)
  {
    buf[2] = (-a2 - deadzone) / m;
  }
  else if (deadzone <= a2)
  {
    buf[3] = (a2 - deadzone) / m;
  }
  buf[4] = 0;
  delay(10 * 20 / 16);
}