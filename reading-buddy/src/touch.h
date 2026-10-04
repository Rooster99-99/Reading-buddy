// ============================================================
//  Touch for the 3.2" CYD — works with both versions:
//    resistive  (XPT2046, shares the screen's SPI bus, CS = 33)
//    capacitive (GT911 over I2C: SDA 33, SCL 32, INT 21, RST 25)
//  Raw readings are turned into screen positions by a quick
//  3-tap calibration that runs on first boot.
// ============================================================
#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

#if defined(TOUCH_CAPACITIVE)
#include <Wire.h>
#define GT_SDA 33
#define GT_SCL 32
#define GT_INT 21
#define GT_RST 25

static uint8_t gtAddr = 0x5D;

static bool gtRead(uint16_t reg, uint8_t* buf, uint8_t len) {
  Wire.beginTransmission(gtAddr);
  Wire.write(reg >> 8); Wire.write(reg & 0xFF);
  if (Wire.endTransmission() != 0) return false;
  if (Wire.requestFrom(gtAddr, len) != len) return false;
  for (uint8_t i = 0; i < len; i++) buf[i] = Wire.read();
  return true;
}

static void gtWrite(uint16_t reg, uint8_t v) {
  Wire.beginTransmission(gtAddr);
  Wire.write(reg >> 8); Wire.write(reg & 0xFF); Wire.write(v);
  Wire.endTransmission();
}

inline void touchBegin() {
  // Reset sequence that selects I2C address 0x5D
  pinMode(GT_INT, OUTPUT); pinMode(GT_RST, OUTPUT);
  digitalWrite(GT_INT, LOW); digitalWrite(GT_RST, LOW); delay(10);
  digitalWrite(GT_RST, HIGH); delay(10);
  pinMode(GT_INT, INPUT); delay(60);
  Wire.begin(GT_SDA, GT_SCL, 400000);
  uint8_t id[4];
  if (!gtRead(0x8140, id, 4)) { gtAddr = 0x14; }   // some boards come up on 0x14
}

// Returns true while a finger is down; fills raw x/y.
inline bool touchRaw(int& x, int& y) {
  static bool down = false;
  static int lx = 0, ly = 0;
  uint8_t st;
  if (gtRead(0x814E, &st, 1) && (st & 0x80)) {       // new report ready
    uint8_t n = st & 0x0F;
    if (n > 0) {
      uint8_t b[4];
      if (gtRead(0x8150, b, 4)) { lx = b[0] | (b[1] << 8); ly = b[2] | (b[3] << 8); }
      down = true;
    } else {
      down = false;
    }
    gtWrite(0x814E, 0);
  }
  x = lx; y = ly;
  return down;
}

#else  // resistive (default)

inline void touchBegin() {}

inline bool touchRaw(int& x, int& y) {
  if (tft.getTouchRawZ() < 350) return false;
  uint16_t rx, ry;
  tft.getTouchRaw(&rx, &ry);
  x = rx; y = ry;
  return true;
}
#endif

// ------------------------------------------------------------
//  Calibration: screen = a * raw + b (with optional axis swap)
// ------------------------------------------------------------
struct TouchCal { uint8_t swap; float ax, bx, ay, by; };
static TouchCal tcal = { 0, 320.0f / 4096, 0, 240.0f / 4096, 0 };

inline bool touchDown() { int x, y; return touchRaw(x, y); }

inline bool touchRead(int& sx, int& sy) {
  int rx, ry;
  if (!touchRaw(rx, ry)) return false;
  float u = tcal.swap ? ry : rx;
  float v = tcal.swap ? rx : ry;
  sx = constrain((int)(tcal.ax * u + tcal.bx), 0, 319);
  sy = constrain((int)(tcal.ay * v + tcal.by), 0, 239);
  return true;
}

// Waits for a tap and returns the averaged raw position.
static void rawTap(int& x, int& y) {
  while (touchDown()) delay(10);
  delay(80);
  while (!touchDown()) delay(10);
  delay(60);                                   // let it settle
  long sx = 0, sy = 0; int n = 0;
  int rx, ry;
  while (n < 12 && touchRaw(rx, ry)) { sx += rx; sy += ry; n++; delay(15); }
  if (n == 0) { touchRaw(rx, ry); sx = rx; sy = ry; n = 1; }
  x = sx / n; y = sy / n;
  while (touchDown()) delay(10);
  delay(80);
}

inline void runCalibration(uint16_t bg, uint16_t fg, uint16_t dot) {
  const int TX[3] = { 30, 290, 30 };
  const int TY[3] = { 30, 30, 210 };
  while (true) {
    int rx[3], ry[3];
    for (int i = 0; i < 3; i++) {
      tft.fillScreen(bg);
      tft.setTextColor(fg);
      tft.setFreeFont(&FreeSansBold12pt7b);
      tft.setTextDatum(MC_DATUM);
      tft.drawString("Grown-up setup", 160, 95);
      tft.setFreeFont(&FreeSans9pt7b);
      tft.drawString("Tap the center of the red dot", 160, 130);
      tft.drawString(String(i + 1) + " of 3", 160, 155);
      tft.fillCircle(TX[i], TY[i], 9, dot);
      tft.drawCircle(TX[i], TY[i], 15, dot);
      rawTap(rx[i], ry[i]);
    }
    // Which raw axis moved when we went left -> right?
    uint8_t swap = abs(ry[1] - ry[0]) > abs(rx[1] - rx[0]);
    float u0 = swap ? ry[0] : rx[0], u1 = swap ? ry[1] : rx[1];
    float v0 = swap ? rx[0] : ry[0], v2 = swap ? rx[2] : ry[2];
    if (fabsf(u1 - u0) < 20 || fabsf(v2 - v0) < 20) continue;   // bad taps, try again
    tcal.swap = swap;
    tcal.ax = (TX[1] - TX[0]) / (u1 - u0);  tcal.bx = TX[0] - tcal.ax * u0;
    tcal.ay = (TY[2] - TY[0]) / (v2 - v0);  tcal.by = TY[0] - tcal.ay * v0;
    return;
  }
}
