// ============================================================
//  Reading Buddy  —  word practice for the 3.2" Cheap Yellow Display
//
//  1. A word appears in big, plain letters. She tries to read it.
//  2. "I read it!"  -> earns a star, word shows up less often.
//  3. "Help me"     -> word splits into colored syllables (with
//                      underlines to chunk it) and sneaky letters
//                      turn red. Word comes back more often.
//  Words she struggles with repeat; words she knows fade out.
//
//  Grown-up controls:
//   - Hold the blue top bar 3 seconds  -> reset stars/progress
//   - Hold your finger on the screen while plugging in -> redo touch setup
// ============================================================
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Preferences.h>
#include "touch.h"
#include "words.h"

// ---- CYD onboard RGB LED (active LOW) ----
#define LED_R 4
#define LED_G 16
#define LED_B 17

TFT_eSPI tft;
Preferences prefs;

// ---- Colors ----
constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
const uint16_t C_BG        = rgb(255, 248, 230);  // soft cream, easier on eyes than white
const uint16_t C_BAR       = rgb(60, 90, 160);
const uint16_t C_TEXT      = rgb(30, 30, 40);
const uint16_t C_SOFT      = rgb(110, 110, 120);
const uint16_t C_TRICKY    = rgb(210, 30, 30);
const uint16_t C_TRICKY_BG = rgb(255, 222, 130);
const uint16_t C_HELP      = rgb(245, 150, 30);
const uint16_t C_GOOD      = rgb(40, 165, 80);
const uint16_t C_NEXT      = rgb(60, 120, 220);
const uint16_t C_GOLD      = rgb(255, 196, 0);
const uint16_t SYL_COLORS[] = { rgb(20, 90, 210), rgb(0, 135, 70), rgb(140, 50, 175) };

// ---- Layout (landscape 320x240) ----
const int W = 320, H = 240;
const int TOP_H = 32;
const int BTN_Y = 182, BTN_H = 52;
const int SYL_GAP = 18;

// ---- Learning boxes (simple Leitner system) ----
// box 0 = needs lots of practice ... box 4 = learned
const uint8_t MAX_BOX = 4;
const uint8_t BOX_WEIGHT[] = { 16, 8, 4, 2, 1 };
uint8_t boxes[NUM_WORDS];
uint32_t stars = 0;

// ---- Current word ----
struct Letter { char c; uint8_t syl; bool tricky; };
Letter letters[40];
int nLetters = 0, nSyl = 1;
bool hasTricky = false;
int current = -1;
bool helpShown = false;

const char* PRAISE[] = { "Great job!", "You did it!", "Awesome!", "Super reading!", "Way to go!", "Nice work!" };
const int NUM_PRAISE = sizeof(PRAISE) / sizeof(PRAISE[0]);

// ============================================================
//  Storage
// ============================================================
uint32_t hashWords() {
  uint32_t h = 2166136261u;
  for (int i = 0; i < NUM_WORDS; i++)
    for (const char* p = WORDS[i]; ; p++) { h = (h ^ (uint8_t)*p) * 16777619u; if (!*p) break; }
  return h;
}

void saveProgress() {
  prefs.putBytes("boxes", boxes, NUM_WORDS);
  prefs.putUInt("stars", stars);
}

void resetProgress() {
  memset(boxes, 0, sizeof(boxes));
  stars = 0;
  prefs.putUInt("hash", hashWords());
  saveProgress();
}

void loadProgress() {
  if (prefs.getUInt("hash", 0) != hashWords() ||
      prefs.getBytes("boxes", boxes, NUM_WORDS) != (size_t)NUM_WORDS) {
    resetProgress();   // word list changed (or first boot)
  } else {
    stars = prefs.getUInt("stars", 0);
  }
}

int learnedCount() {
  int n = 0;
  for (int i = 0; i < NUM_WORDS; i++) if (boxes[i] >= MAX_BOX) n++;
  return n;
}

// ============================================================
//  Word selection & parsing
// ============================================================
int pickWord() {
  uint32_t total = 0;
  for (int i = 0; i < NUM_WORDS; i++)
    if (i != current || NUM_WORDS == 1) total += BOX_WEIGHT[boxes[i]];
  uint32_t r = esp_random() % total;
  for (int i = 0; i < NUM_WORDS; i++) {
    if (i == current && NUM_WORDS > 1) continue;
    uint32_t w = BOX_WEIGHT[boxes[i]];
    if (r < w) return i;
    r -= w;
  }
  return 0;
}

void parseWord(const char* w) {
  nLetters = 0; hasTricky = false;
  uint8_t syl = 0; bool tr = false;
  for (const char* p = w; *p && nLetters < 39; p++) {
    if (*p == '-') { syl++; continue; }
    if (*p == '[') { tr = true; continue; }
    if (*p == ']') { tr = false; continue; }
    letters[nLetters++] = { *p, syl, tr };
    if (tr) hasTricky = true;
  }
  nSyl = syl + 1;
}

// ============================================================
//  Drawing
// ============================================================
int charW(char c) { char s[2] = { c, 0 }; return tft.textWidth(s); }

void drawStar(int cx, int cy, int r, uint16_t color) {
  float px[10], py[10];
  for (int i = 0; i < 10; i++) {
    float a = -PI / 2 + i * PI / 5;
    float rr = (i % 2 == 0) ? r : r * 0.45f;
    px[i] = cx + rr * cosf(a); py[i] = cy + rr * sinf(a);
  }
  for (int i = 0; i < 10; i++) {
    int j = (i + 1) % 10;
    tft.fillTriangle(cx, cy, px[i], py[i], px[j], py[j], color);
  }
}

void drawTopBar() {
  tft.fillRect(0, 0, W, TOP_H, C_BAR);
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("Reading Buddy", 8, TOP_H / 2);

  char buf[24];
  snprintf(buf, sizeof(buf), "%d/%d learned", learnedCount(), NUM_WORDS);
  tft.setTextFont(2);
  tft.setTextDatum(MR_DATUM);
  tft.drawString(buf, 240, TOP_H / 2);

  drawStar(258, TOP_H / 2, 10, C_GOLD);
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)stars);
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextDatum(ML_DATUM);
  tft.drawString(buf, 272, TOP_H / 2);
}

void drawButton(int x, int w, const char* label, uint16_t color) {
  tft.fillRoundRect(x, BTN_Y, w, BTN_H, 12, color);
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(&FreeSansBold12pt7b);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(label, x + w / 2, BTN_Y + BTN_H / 2);
}

void drawButtons() {
  tft.fillRect(0, BTN_Y - 2, W, H - BTN_Y + 2, C_BG);
  if (!helpShown) {
    drawButton(8, 148, "Help me", C_HELP);
    drawButton(164, 148, "I read it!", C_GOOD);
  } else {
    drawButton(8, 304, "Next word", C_NEXT);
  }
}

const GFXfont* WORD_FONTS[] = { &FreeSansBold24pt7b, &FreeSansBold18pt7b, &FreeSansBold12pt7b };

void drawWord() {
  tft.fillRect(0, TOP_H, W, BTN_Y - TOP_H - 2, C_BG);

  // Pick the biggest font that fits
  int total = 0, fi;
  for (fi = 0; fi < 3; fi++) {
    tft.setFreeFont(WORD_FONTS[fi]);
    total = 0;
    for (int i = 0; i < nLetters; i++) total += charW(letters[i].c);
    if (helpShown) total += (nSyl - 1) * SYL_GAP;
    if (total <= W - 16) break;
  }
  if (fi > 2) fi = 2;
  tft.setFreeFont(WORD_FONTS[fi]);
  int fh = tft.fontHeight();
  int baseline = helpShown ? 106 : 118;

  // Letter positions (extra gap between syllables in help mode)
  int xs[40], ws[40];
  int cx = (W - total) / 2;
  for (int i = 0; i < nLetters; i++) {
    if (helpShown && i > 0 && letters[i].syl != letters[i - 1].syl) cx += SYL_GAP;
    xs[i] = cx; ws[i] = charW(letters[i].c);
    cx += ws[i];
  }

  if (helpShown) {
    // Highlight sneaky letters
    for (int i = 0; i < nLetters; i++) {
      if (!letters[i].tricky) continue;
      int j = i;
      while (j + 1 < nLetters && letters[j + 1].tricky && letters[j + 1].syl == letters[i].syl) j++;
      int top = baseline - (int)(fh * 0.66f);
      tft.fillRoundRect(xs[i] - 3, top, xs[j] + ws[j] - xs[i] + 6, (int)(fh * 0.82f), 6, C_TRICKY_BG);
      i = j;
    }
    // Underline each syllable in its color, with a dot between syllables
    for (int s = 0; s < nSyl; s++) {
      int a = -1, b = -1;
      for (int i = 0; i < nLetters; i++) if (letters[i].syl == s) { if (a < 0) a = i; b = i; }
      if (a < 0) continue;
      tft.fillRoundRect(xs[a], baseline + 14, xs[b] + ws[b] - xs[a], 5, 2, SYL_COLORS[s % 3]);
      if (s > 0) tft.fillCircle(xs[a] - SYL_GAP / 2, baseline - fh / 5, 3, C_SOFT);
    }
  }

  // The letters themselves
  tft.setTextDatum(L_BASELINE);
  for (int i = 0; i < nLetters; i++) {
    uint16_t col = C_TEXT;
    if (helpShown) col = letters[i].tricky ? C_TRICKY : SYL_COLORS[letters[i].syl % 3];
    tft.setTextColor(col);
    char s[2] = { letters[i].c, 0 };
    tft.drawString(s, xs[i], baseline);
  }

  // Coaching hints
  if (helpShown) {
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(C_TEXT);
    char buf[48];
    if (nSyl == 1) snprintf(buf, sizeof(buf), "1 beat - say it slowly");
    else           snprintf(buf, sizeof(buf), "%d beats - clap each part!", nSyl);
    tft.drawString(buf, W / 2, 134);
    if (hasTricky) {
      tft.setTextColor(C_TRICKY);
      tft.drawString("Red letters make a sneaky sound", W / 2, 156);
    }
  }
}

void showWord() {
  helpShown = false;
  parseWord(WORDS[current]);
  drawTopBar();
  drawWord();
  drawButtons();
}

void celebrate() {
  digitalWrite(LED_G, LOW);
  tft.fillRect(0, TOP_H, W, H - TOP_H, C_BG);
  drawTopBar();
  tft.setFreeFont(&FreeSansBold18pt7b);
  tft.setTextDatum(MC_DATUM);
  if (stars > 0 && stars % 10 == 0) {           // big party every 10 stars
    for (int i = 0; i < 12; i++)
      drawStar(20 + esp_random() % (W - 40), TOP_H + 20 + esp_random() % 150,
               8 + esp_random() % 12, SYL_COLORS[i % 3]);
    drawStar(W / 2, 100, 42, C_GOLD);
    tft.setTextColor(C_BAR);
    char buf[24];
    snprintf(buf, sizeof(buf), "%lu stars!", (unsigned long)stars);
    tft.drawString(buf, W / 2, 180);
    delay(2200);
  } else {
    drawStar(W / 2, 95, 38, C_GOLD);
    tft.setTextColor(C_GOOD);
    tft.drawString(PRAISE[esp_random() % NUM_PRAISE], W / 2, 168);
    delay(900);
  }
  digitalWrite(LED_G, HIGH);
}

// ============================================================
//  Touch helpers
// ============================================================
void waitRelease() {
  uint32_t lastDown = millis();
  while (millis() - lastDown < 60) { if (touchDown()) lastDown = millis(); delay(10); }
}

void calibrateAndSave() {
  runCalibration(C_BG, C_TEXT, C_TRICKY);
  prefs.putBytes("cal", &tcal, sizeof(tcal));
}

bool confirmReset() {
  tft.fillScreen(C_BG);
  tft.setFreeFont(&FreeSansBold12pt7b);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_TEXT);
  tft.drawString("Start over?", W / 2, 60);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.drawString("This clears stars and progress.", W / 2, 100);
  drawButton(8, 148, "Cancel", C_NEXT);
  drawButton(164, 148, "Reset", C_TRICKY);
  waitRelease();
  int x, y;
  while (true) {
    if (touchRead(x, y) && y >= BTN_Y - 10) { waitRelease(); return x >= W / 2; }
    delay(20);
  }
}

// ============================================================
void setup() {
  Serial.begin(115200);
  pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT); pinMode(LED_B, OUTPUT);
  digitalWrite(LED_R, HIGH); digitalWrite(LED_G, HIGH); digitalWrite(LED_B, HIGH);

  tft.init();
  tft.setRotation(1);          // landscape
  tft.fillScreen(C_BG);
  touchBegin();

  prefs.begin("reading", false);
  loadProgress();

  // Touch setup on first boot, or if a finger is on the screen at power-up
  delay(300);
  bool haveCal = prefs.getBytes("cal", &tcal, sizeof(tcal)) == sizeof(tcal);
  if (!haveCal || touchDown()) calibrateAndSave();

  current = pickWord();
  showWord();
}

void loop() {
  int x, y;
  if (!touchRead(x, y)) { delay(15); return; }

  // Hold the top bar 3 seconds -> reset progress
  if (y < TOP_H) {
    uint32_t t0 = millis();
    while (touchDown() && millis() - t0 < 3000) delay(20);
    if (millis() - t0 >= 3000) {
      if (confirmReset()) resetProgress();
      current = pickWord();
      showWord();
    }
    waitRelease();
    return;
  }

  if (y < BTN_Y - 10) return;   // taps on the word itself do nothing
  waitRelease();

  if (!helpShown) {
    if (x < W / 2) {                       // "Help me"
      helpShown = true;
      boxes[current] = 0;                  // bring this word back soon
      saveProgress();
      drawTopBar();
      drawWord();
      drawButtons();
    } else {                               // "I read it!"
      if (boxes[current] < MAX_BOX) boxes[current]++;
      stars++;
      saveProgress();
      celebrate();
      current = pickWord();
      showWord();
    }
  } else {                                 // "Next word" after help
    stars++;                               // effort counts too
    saveProgress();
    current = pickWord();
    showWord();
  }
}
