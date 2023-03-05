#include <Arduino.h>
#include <Wire.h>
#define X_PIN PIN_A7
#define Y_PIN PIN_A6
#define VCC_PIN PIN_A3

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

// const int max = 1024;
const int deadzone = 100;
const int high_zone = 7;
const int middle = 200;
// const int center = 512;
const int shift = -8;
const int m = 4;
const int loop_ms = 12;
const int max_ms = 4028;

int last_center_ms = 0;
int last_high_ms = 0;
bool in_dash = false;

void read_analog(int vcc, int a, int *under, int *upper)
{

  int max = vcc;
  int center = vcc / 2;
  if (a < high_zone)
  {
    *under = 5;
  }
  else if (a < middle)
  {
    *under = 3;
  }
  else if (a < center - deadzone)
  {
    *under = 1;
  }
  else
  {
    *under = 0;
  }

  if (max - high_zone < a)
  {
    *upper = 5;
  }
  else if (max - middle < a)
  {
    *upper = 3;
  }
  else if (center + deadzone < a)
  {
    *upper = 1;
  }
  else
  {
    *upper = 0;
  }
}

void loop()
{
  int left = 0;
  int right = 0;
  int up = 0;
  int down = 0;

  int raw_vcc = analogRead(VCC_PIN);
  int raw_x = analogRead(X_PIN);
  int raw_y = analogRead(Y_PIN);
  read_analog(raw_vcc, raw_x, &left, &right);
  read_analog(raw_vcc, raw_y, &down, &up);

  if (left == 0 && right == 0)
  {
    last_center_ms = 0;
  }
  else
  {
    if (last_center_ms < max_ms)
    {
      last_center_ms += loop_ms;
    }
  }

  if (left + right + up + down >= 4)
  {
    if (!in_dash && last_center_ms < 300 && 10 < last_high_ms && last_high_ms < 300)
    {
      in_dash = true;
    }
    if (in_dash)
    {
      left = left * 2;
      right = right * 2;
      up = up * 2;
      down = down * 2;
    }
    last_high_ms = 0;
  }
  else
  {
    in_dash = false;
    if (last_high_ms < max_ms)
    {
      last_high_ms += loop_ms;
    }
  }

  buf[0] = left;
  buf[1] = right;
  buf[2] = down;
  buf[3] = up;
  delay(loop_ms);
}