#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <Update.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <FastLED.h>              // ефекти й кольори
#include <Adafruit_NeoPixel.h>    // вивід на матрицю (перевірено, що працює)
#include <vector>
#include <esp_https_server.h>
#include <esp_core_dump.h>
#include "page.h"
#include "cam.h"
#include "secrets.h"   // Wi-Fi і пароль точки доступу; не в git, зразок у secrets.example.h
#include "cert.h"      // HTTPS-сертифікат; не в git, генерується gen_cert.sh
#include "pacman.h"

// ===== Залізо =====
#define DATA_PIN    13
#define W           16
#define H           16
#define NUM_LEDS    (W * H)
#define FRAME_BYTES (NUM_LEDS * 3)
#define REC_BYTES   (2 + FRAME_BYTES)   // затримка (мс) + кадр RGB
#define HDR_BYTES   6                   // "L16A" + кількість кадрів

// Значення за замовчуванням; змінюються у вебсторінці (Налаштування)
const char* AP_SSID  = "Lamp";          // запасна точка доступу, якщо Wi-Fi недоступний
const char* HOSTNAME = "lamp";          // http://lamp.local

CRGB leds[NUM_LEDS];
Adafruit_NeoPixel strip(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);
WebServer server(80);
Preferences prefs;
WiFiClient net;
PubSubClient mqtt(net);

struct Config {
  String   ssid, pass, mqttHost, mqttUser, mqttPass;
  uint16_t mqttPort = 1883;
  uint8_t  rot = 0;                     // поворот, x90°
  bool     flip = false;                // дзеркало
  bool     serp = true;                 // матриця "змійкою"
  uint16_t maxMa = 2000;                // ліміт струму
} cfg;

// ===== Режими =====
enum { M_SOLID, M_RAINBOW, M_FIRE, M_CONFETTI, M_BREATHE, M_HEART, M_RAIN, M_SNOW, M_PACMAN, M_SUNRISE, M_SUNSET, MODE_COUNT };
const uint8_t M_ANIM = MODE_COUNT;      // програвання завантаженого файлу
const uint8_t M_DRAW = MODE_COUNT + 1;  // живе малювання з вебсторінки (після перезавантаження не відновлюється)
const char* MODE_NAMES[MODE_COUNT] = {"Колір", "Веселка", "Вогонь", "Конфетті", "Дихання", "Серце", "Матриця", "Сніг", "Пакман", "Світанок", "Захід"};
const char* ANIM_PREFIX = "▶ ";         // так анімації називаються в списку ефектів HA

bool     power      = true;
uint8_t  mode       = M_SOLID;
uint8_t  brightness = 80;
uint32_t color      = 0xFF9329;         // теплий білий
String   animName;

struct Anim { String name; uint16_t frames; };
std::vector<Anim> anims;

uint16_t xyMap[NUM_LEDS];
uint8_t  drawBuf[FRAME_BYTES];
uint8_t  gamma8[256];
uint32_t testUntil = 0, dirtyAt = 0;
bool     fxStart = true;                // перший кадр ефекту після перемикання
bool     apMode = false;
String   devId;

// ===== Геометрія =====
// Логічні координати: x вправо, y вниз від верхнього лівого кута
void buildMap() {
  for (uint8_t y = 0; y < H; y++)
    for (uint8_t x = 0; x < W; x++) {
      uint8_t tx = x, ty = y;
      if (cfg.flip) tx = W - 1 - tx;
      for (uint8_t r = 0; r < cfg.rot; r++) { uint8_t t = tx; tx = W - 1 - ty; ty = t; }
      if (cfg.serp && (ty & 1)) tx = W - 1 - tx;
      xyMap[y * W + x] = ty * W + tx;
    }
}
inline uint16_t XY(uint8_t x, uint8_t y) { return xyMap[y * W + x]; }

// ===== Ефекти =====
void effectSolid() { fill_solid(leds, NUM_LEDS, CRGB(color)); }

void effectRainbow() {
  static uint8_t hue = 0;
  hue++;
  for (uint8_t y = 0; y < H; y++)
    for (uint8_t x = 0; x < W; x++)
      leds[XY(x, y)] = CHSV(hue + (x + y) * 8, 255, 255);
}

void effectFire() {               // Fire2012 по кожному стовпчику, знизу вгору
  static uint8_t heat[W][H];
  for (uint8_t x = 0; x < W; x++) {
    for (uint8_t y = 0; y < H; y++)
      heat[x][y] = qsub8(heat[x][y], random8(0, (55 * 10) / H + 2));
    for (uint8_t y = H - 1; y >= 2; y--)
      heat[x][y] = (heat[x][y - 1] + heat[x][y - 2] + heat[x][y - 2]) / 3;
    if (random8() < 120) {
      uint8_t y = random8(3);
      heat[x][y] = qadd8(heat[x][y], random8(160, 255));
    }
    for (uint8_t y = 0; y < H; y++)
      leds[XY(x, H - 1 - y)] = HeatColor(heat[x][y]);
  }
}

void effectConfetti() {
  fadeToBlackBy(leds, NUM_LEDS, 10);
  leds[random16(NUM_LEDS)] += CHSV(random8(), 200, 255);
}

void effectBreathe() {
  fill_solid(leds, NUM_LEDS, CRGB(color));
  nscale8_video(leds, NUM_LEDS, beatsin8(12, 30, 255));
}

// Чи настав час наступного кадру; при відставанні не надолужуємо, а починаємо відлік заново
bool fxDue(uint32_t& next, uint16_t ms) {
  uint32_t now = millis();
  if (!fxStart && (int32_t)(now - next) < 0) return false;
  next = (fxStart || (int32_t)(now - next) > 500) ? now + ms : next + ms;
  return true;
}

void effectHeart() {              // серце, що б'ється: подвійний удар і пауза
  static const float BEAT[] = {1.0, 1.08, 1.15, 1.08, 1.0, 1.0, 1.06, 1.1, 1.06, 1.0, 1.0, 1.0, 1.0, 1.0};
  static uint8_t i = 0;
  static uint32_t next = 0;
  if (fxStart) i = 0;
  if (!fxDue(next, 70)) return;
  float k = 6.2f * BEAT[i];
  for (uint8_t y = 0; y < H; y++)
    for (uint8_t x = 0; x < W; x++) {
      float u = (x - 7.5f) / k, v = -(y - 7.0f) / k, t = u * u + v * v - 0.5f;
      leds[XY(x, y)] = t * t * t - u * u * v * v * v * 0.9f <= 0 ? CRGB(255, 0, 6) : CRGB::Black;
    }
  i = (i + 1) % (sizeof(BEAT) / sizeof(BEAT[0]));
}

void effectRain() {               // цифровий дощ, як у «Матриці»: у кожному стовпчику своя крапля
  struct Drop { int16_t head; uint8_t speed, len; };   // голова в 1/8 пікселя
  static Drop d[W];
  static uint32_t next = 0;
  if (fxStart) for (auto& p : d) p = {(int16_t)(-random8(H) * 8), (uint8_t)random8(2, 7), (uint8_t)random8(5, 11)};
  if (!fxDue(next, 40)) return;
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  for (uint8_t x = 0; x < W; x++) {
    Drop& p = d[x];
    p.head += p.speed;
    int16_t hy = p.head / 8;
    if (hy - p.len >= H) p = {(int16_t)(-random8(4, 24) * 8), (uint8_t)random8(2, 7), (uint8_t)random8(5, 11)};
    for (uint8_t k = 0; k < p.len; k++) {
      int16_t y = hy - k;
      if (y < 0 || y >= H) continue;
      uint8_t g = 255 - k * 255 / p.len;
      leds[XY(x, y)] = k ? CRGB(0, scale8(g, g), 0) : CRGB(150, 255, 150);
    }
  }
}

void effectSnow() {               // снігопад: ближні сніжинки яскравіші й швидші
  struct Flake { uint8_t x; int16_t y; bool near; };   // y в 1/16 пікселя
  static Flake f[26];
  static uint32_t next = 0;
  if (fxStart) for (auto& p : f) p = {random8(W), (int16_t)random16(H * 16), random8() < 128};
  if (!fxDue(next, 40)) return;
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  for (auto& p : f) {
    p.y += p.near ? 6 : 3;
    if (random8() < 6) p.x = (p.x + (random8() & 1 ? 1 : W - 1)) % W;   // вітер
    if (p.y >= H * 16) p = {random8(W), (int16_t)-random8(32), random8() < 128};
    if (p.y >= 0) leds[XY(p.x, p.y / 16)] = p.near ? CRGB(255, 255, 255) : CRGB(56, 56, 100);
  }
}

// Пакман: покадрова копія loop() з PacChaseLogo.ino — вікно 16x16 ковзає по спрайтах 112 пікселів завширшки
#define PAC_W 112
struct PacSprite { const uint8_t* px; const uint32_t* pal; uint16_t len; };
const PacSprite PAC_SPRITES[] = {
  {LogoSprite, LogoPalette, sizeof(LogoSprite)},
  {PacSpriteM, PacPaletteM, sizeof(PacSpriteM)}, {PacSpriteT, PacPaletteT, sizeof(PacSpriteT)}, {PacSpriteB, PacPaletteB, sizeof(PacSpriteB)},
  {PacSpriteM2, PacPaletteM2, sizeof(PacSpriteM2)}, {PacSpriteT2, PacPaletteT2, sizeof(PacSpriteT2)}, {PacSpriteB2, PacPaletteB2, sizeof(PacSpriteB2)},
};
enum { PS_LOGO, PS_M, PS_T, PS_B, PS_M2, PS_T2, PS_B2 };
struct PacStep { uint8_t spr, ox, oy, delay; };
std::vector<PacStep> pacSteps;

void buildPacSteps() {
  auto add = [](uint8_t s, int x, int y, uint8_t d) { pacSteps.push_back({s, (uint8_t)x, (uint8_t)y, d}); };
  auto pacChase = [&](int start) {
    for (int x = start; x < PAC_W - 16; x += 4) { add(PS_M, x, 0, 70); add(PS_T, x + 1, 0, 70); add(PS_M, x + 2, 0, 70); add(PS_B, x + 3, 0, 70); }
  };
  auto ghostChase = [&] {
    for (int x = PAC_W - 20; x > 15; x -= 4) { add(PS_M2, x, 0, 70); add(PS_T2, x - 1, 0, 70); add(PS_M2, x - 2, 0, 70); add(PS_B2, x - 3, 0, 70); }
  };
  for (int y = 16; y > 0; y--) add(PS_LOGO, 0, y, 40);           // логотип виїжджає знизу
  for (int x = 0; x < PAC_W - 15; x++) add(PS_LOGO, x, 0, 60);   // і прокручується
  pacChase(0); ghostChase();
  for (int i = 0; i < 3; i++) { pacChase(14); ghostChase(); }
}

void effectPacman() {
  static uint16_t i = 0;
  static uint32_t next = 0;
  if (pacSteps.empty()) buildPacSteps();
  if (fxStart) i = 0;
  const PacStep& s = pacSteps[i];
  if (!fxDue(next, s.delay)) return;
  const PacSprite& sp = PAC_SPRITES[s.spr];
  for (uint8_t y = 0; y < H; y++)
    for (uint8_t x = 0; x < W; x++) {
      uint16_t pos = ((y + s.oy) * PAC_W + (s.ox + x) % PAC_W) % (PAC_W * 32);
      leds[XY(x, y)] = CRGB(sp.pal[pos < sp.len ? sp.px[pos] : 0]);
    }
  i = (i + 1) % pacSteps.size();
}

// Небо з сонцем: k = 0 ніч (сонце під горизонтом), 255 день (сонце вгорі, тепле небо)
void drawSky(uint8_t k) {
  uint8_t e = ease8InOutQuad(k);
  float sy = H + 3 - (H - 2) * k / 255.0f;              // центр сонця: з-під низу до y = 5
  CRGB sun = blend(CRGB(255, 30, 0), CRGB(255, 220, 140), e);
  for (uint8_t y = 0; y < H; y++) {
    uint8_t h = y * 255 / (H - 1);                        // 0 вгорі, 255 біля горизонту
    CRGB sky = blend(blend(CRGB(0, 0, 10), CRGB(25, 0, 30), h), blend(CRGB(255, 150, 50), CRGB(255, 80, 10), h), e);
    for (uint8_t x = 0; x < W; x++) {
      float dx = x - 7.5f, dy = y - sy, d = sqrtf(dx * dx + dy * dy);
      CRGB c = sky;
      if (d < 3.2f) c = sun;
      else if (d < 9) nblend(c, sun, (uint8_t)((9 - d) * 28));   // ореол
      leds[XY(x, y)] = c;
    }
  }
}

// Тривалість світанку й заходу: 15 хв, або скільки передали в MQTT-команді ("duration", с) — напр. від заходу до 23:00
#define SKY_MS_DEFAULT (15UL * 60 * 1000)
uint32_t skyMs = SKY_MS_DEFAULT;

// Прогрес ефекту від запуску: 0..255 за skyMs
uint8_t skyProgress(uint32_t& start) {
  if (fxStart) start = millis();
  return min<uint32_t>((millis() - start) / (skyMs / 255), 255);
}

// Світанок: нічне небо теплішає, сонце піднімається знизу; далі тримає останній кадр
void effectSunrise() {
  static uint32_t start = 0, next = 0;
  uint8_t k = skyProgress(start);
  if (!fxDue(next, 100)) return;
  drawSky(k);
}

// Захід: сонце сідає, небо темнішає, в останні ~10% лампа плавно гасне й вимикається
void changed();
void effectSunset() {
  static uint32_t start = 0, next = 0;
  uint8_t k = skyProgress(start);
  if (!fxDue(next, 100)) return;
  drawSky(255 - k);
  if (k > 230) nscale8_video(leds, NUM_LEDS, (255 - k) * 255 / 25);
  if (k == 255) { power = false; changed(); }
}

void (*EFFECTS[MODE_COUNT])() = {effectSolid, effectRainbow, effectFire, effectConfetti, effectBreathe,
                                 effectHeart, effectRain, effectSnow, effectPacman, effectSunrise, effectSunset};

// Тест орієнтації: верхній рядок червоний, лівий стовпчик зелений, кут білий
void drawTest() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  for (uint8_t i = 0; i < W; i++) leds[XY(i, 0)] = CRGB::Red;
  for (uint8_t i = 0; i < H; i++) leds[XY(0, i)] = CRGB::Green;
  leds[XY(0, 0)] = CRGB::White;
}

// ===== Анімації з файлів =====
// Формат: "L16A", u16 кількість кадрів, далі кадри: u16 затримка мс + 16*16*3 байт RGB (рядками зверху)
File     animFile;
uint16_t animCount = 0, animIdx = 0;
uint32_t animNext = 0;

String animPath(const String& n) { return "/a/" + n + ".bin"; }

String cleanName(const String& n) {
  String o;
  for (unsigned i = 0; i < n.length(); i++) {
    uint8_t c = n[i];
    if (isalnum(c) || c == '-' || c == '_' || c >= 0x80) o += (char)c;
  }
  return o;
}

void scanAnims() {
  anims.clear();
  File dir = LittleFS.open("/a");
  if (!dir) return;
  for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
    String n = f.name();
    if (n.endsWith(".bin") && f.size() > HDR_BYTES)
      anims.push_back({n.substring(0, n.length() - 4), (uint16_t)((f.size() - HDR_BYTES) / REC_BYTES)});
    f.close();
  }
}

void closeAnim() { if (animFile) animFile.close(); }

bool openAnim() {
  closeAnim();
  if (animName.isEmpty()) return false;
  animFile = LittleFS.open(animPath(animName), "r");
  if (!animFile) return false;
  uint8_t h[HDR_BYTES];
  if (animFile.read(h, HDR_BYTES) != HDR_BYTES || memcmp(h, "L16A", 4) != 0) { closeAnim(); return false; }
  animCount = h[4] | (h[5] << 8);
  if (!animCount || animFile.size() != HDR_BYTES + (size_t)animCount * REC_BYTES) { closeAnim(); return false; }
  animIdx = 0;
  animNext = millis();
  return true;
}

void animStep() {
  if (!animFile) { fill_solid(leds, NUM_LEDS, CRGB::Black); return; }
  if ((int32_t)(millis() - animNext) < 0) return;
  if (animIdx >= animCount) { animFile.seek(HDR_BYTES); animIdx = 0; }
  static uint8_t buf[REC_BYTES];
  if (animFile.read(buf, REC_BYTES) != REC_BYTES) { animIdx = animCount; return; }
  uint16_t d = buf[0] | (buf[1] << 8);
  for (uint8_t y = 0; y < H; y++)
    for (uint8_t x = 0; x < W; x++) {
      const uint8_t* p = buf + 2 + (y * W + x) * 3;
      leds[XY(x, y)] = CRGB(gamma8[p[0]], gamma8[p[1]], gamma8[p[2]]);
    }
  animIdx++;
  animNext = millis() + (animCount == 1 ? 60000 : max<uint16_t>(d, 20));
}

// ===== Керування станом =====
void setMode(uint8_t m) {
  if (m >= MODE_COUNT) return;
  skyMs = SKY_MS_DEFAULT;
  if (m != mode) fxStart = true;
  mode = m;
  closeAnim();
}

bool playAnim(const String& n) {
  animName = cleanName(n);
  mode = M_ANIM;
  if (openAnim()) return true;
  setMode(M_SOLID);
  return false;
}

String effectName() {
  if (mode == M_ANIM) return ANIM_PREFIX + animName;
  if (mode == M_DRAW) return "";
  return MODE_NAMES[mode];
}

void drawStep() {
  for (uint8_t y = 0; y < H; y++)
    for (uint8_t x = 0; x < W; x++) {
      const uint8_t* p = drawBuf + (y * W + x) * 3;
      leds[XY(x, y)] = CRGB(gamma8[p[0]], gamma8[p[1]], gamma8[p[2]]);
    }
}

void setEffect(const String& e) {
  for (uint8_t i = 0; i < MODE_COUNT; i++)
    if (e == MODE_NAMES[i]) { setMode(i); return; }
  if (e.startsWith(ANIM_PREFIX)) playAnim(e.substring(strlen(ANIM_PREFIX)));
}

void setColor(uint32_t c) {
  color = c & 0xFFFFFF;
  if (mode != M_SOLID && mode != M_BREATHE) setMode(M_SOLID);
}

void publishState();
void changed() {
  dirtyAt = millis() | 1;                 // зберегти у флеш із затримкою, щоб не зношувати її повзунком
  publishState();
}

void showLeds() {
  bool on = power || (int32_t)(testUntil - millis()) > 0;
  // Ліміт струму. Яскравість рахується від пікової потужності кадрів, що повільно спадає (~4 с), тож не скаче
  // з кожним кадром «Вогню»; повзунок діє завжди: 255 = максимум, який дозволяє ліміт для поточного ефекту.
  // Після вмикання чи зміни режиму ліміт наростає ~1 с — різкий стрибок струму просаджує живлення (brownout).
  static uint32_t peak = 0, ramp = 0;
  static uint8_t lastMode = 255;
  uint32_t full = calculate_unscaled_power_mW(leds, NUM_LEDS);          // потужність кадру на яскравості 255
  peak = max(full, peak - peak / 256);
  if (!on || mode != lastMode) ramp = 0;
  lastMode = mode;
  ramp = min<uint32_t>(5 * cfg.maxMa, ramp + 150);
  uint8_t bMax = min<uint32_t>(255, (uint64_t)ramp * 256 / max<uint32_t>(peak, 1));
  uint8_t b = on ? (uint16_t)bMax * brightness / 255 : 0;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    CRGB c = leds[i];
    c.nscale8_video(b);
    strip.setPixelColor(i, c.r, c.g, c.b);
  }
  strip.show();
}

// ===== Збереження =====
void loadAll() {
  prefs.begin("lamp", true);
  power      = prefs.getBool("power", true);
  mode       = prefs.getUChar("mode", M_SOLID);
  brightness = prefs.getUChar("bri", 80);
  color      = prefs.getUInt("color", 0xFF9329);
  animName   = prefs.getString("anim", "");
  if (!prefs.isKey("mv") && mode >= 5) mode = mode == 5 ? M_ANIM : M_SOLID;   // старі номери: 5 анімація, 6 малювання
  else if (prefs.getUChar("mv", 0) == 1 && mode >= M_SUNRISE) mode += 2;     // v1: анімація була 9, малювання 10
  else if (prefs.getUChar("mv", 0) == 2 && mode >= M_SUNSET) mode++;         // v2: анімація була 10, малювання 11
  prefs.end();

  prefs.begin("cfg", true);
  cfg.ssid     = prefs.getString("ssid", DEF_SSID);
  cfg.pass     = prefs.getString("pass", DEF_PASS);
  cfg.mqttHost = prefs.getString("mhost", "");
  cfg.mqttPort = prefs.getUShort("mport", 1883);
  cfg.mqttUser = prefs.getString("muser", "");
  cfg.mqttPass = prefs.getString("mpass", "");
  cfg.rot      = prefs.getUChar("rot", 0) % 4;
  cfg.flip     = prefs.getBool("flip", false);
  cfg.serp     = prefs.getBool("serp", true);
  cfg.maxMa    = prefs.getUShort("maxma", 2000);
  prefs.end();
}

void saveState() {
  prefs.begin("lamp", false);
  prefs.putBool("power", power);
  prefs.putUChar("mode", mode);
  prefs.putUChar("bri", brightness);
  prefs.putUInt("color", color);
  prefs.putString("anim", animName);
  prefs.putUChar("mv", 3);
  prefs.end();
}

void saveConfig() {
  prefs.begin("cfg", false);
  prefs.putString("ssid", cfg.ssid);
  prefs.putString("pass", cfg.pass);
  prefs.putString("mhost", cfg.mqttHost);
  prefs.putUShort("mport", cfg.mqttPort);
  prefs.putString("muser", cfg.mqttUser);
  prefs.putString("mpass", cfg.mqttPass);
  prefs.putUChar("rot", cfg.rot);
  prefs.putBool("flip", cfg.flip);
  prefs.putBool("serp", cfg.serp);
  prefs.putUShort("maxma", cfg.maxMa);
  prefs.end();
}

// ===== MQTT / Home Assistant =====
String topic(const char* s) { return "lamp/" + devId + "/" + s; }

void publishDiscovery() {
  if (!mqtt.connected()) return;
  JsonDocument d;
  d["name"]       = nullptr;              // сутність бере назву пристрою
  d["uniq_id"]    = devId;
  d["schema"]     = "json";
  d["cmd_t"]      = topic("set");
  d["stat_t"]     = topic("state");
  d["avty_t"]     = topic("avail");
  d["brightness"] = true;
  d["sup_clrm"].to<JsonArray>().add("rgb");
  d["effect"]     = true;
  JsonArray fx = d["fx_list"].to<JsonArray>();
  for (auto n : MODE_NAMES) fx.add(n);
  for (auto& a : anims) fx.add(ANIM_PREFIX + a.name);
  JsonObject dev = d["dev"].to<JsonObject>();
  dev["ids"].to<JsonArray>().add(devId);
  dev["name"] = "Лампа";
  dev["mf"]   = "DIY";
  dev["mdl"]  = "ESP32 + WS2812B 16x16";
  dev["cu"]   = "http://" + WiFi.localIP().toString() + "/";
  String s;
  serializeJson(d, s);
  mqtt.publish(("homeassistant/light/" + devId + "/config").c_str(), s.c_str(), true);
}

void publishState() {
  if (!mqtt.connected()) return;
  JsonDocument d;
  d["state"]      = power ? "ON" : "OFF";
  d["brightness"] = brightness;
  d["color_mode"] = "rgb";
  JsonObject c = d["color"].to<JsonObject>();
  c["r"] = (color >> 16) & 255;
  c["g"] = (color >> 8) & 255;
  c["b"] = color & 255;
  if (mode != M_DRAW) d["effect"] = effectName();
  String s;
  serializeJson(d, s);
  mqtt.publish(topic("state").c_str(), s.c_str(), true);
}

void onMqtt(char* tp, byte* payload, unsigned int len) {
  if (strcmp(tp, "homeassistant/status") == 0) {          // HA перезапустився
    if (len == 6 && memcmp(payload, "online", 6) == 0) { publishDiscovery(); publishState(); }
    return;
  }
  JsonDocument d;
  if (deserializeJson(d, payload, len)) return;
  if (d["state"].is<const char*>()) power = strcmp(d["state"].as<const char*>(), "ON") == 0;
  if (d["brightness"].is<int>())    brightness = constrain(d["brightness"].as<int>(), 1, 255);
  if (d["color"].is<JsonObject>())
    setColor((uint32_t)(d["color"]["r"] | 0) << 16 | (d["color"]["g"] | 0) << 8 | (d["color"]["b"] | 0));
  if (d["effect"].is<const char*>()) setEffect(d["effect"].as<String>());
  if (d["duration"].is<uint32_t>()) {                      // тривалість Світанку/Заходу, починаємо заново
    skyMs = constrain(d["duration"].as<uint32_t>(), 60UL, 12UL * 3600) * 1000;
    fxStart = true;
  }
  changed();
}

void mqttLoop() {
  if (cfg.mqttHost.isEmpty() || WiFi.status() != WL_CONNECTED) return;
  if (mqtt.connected()) { mqtt.loop(); return; }
  static uint32_t lastTry = 0;
  if (lastTry && millis() - lastTry < 10000) return;
  lastTry = millis() | 1;
  mqtt.setServer(cfg.mqttHost.c_str(), cfg.mqttPort);
  bool ok = mqtt.connect(devId.c_str(),
                         cfg.mqttUser.isEmpty() ? nullptr : cfg.mqttUser.c_str(),
                         cfg.mqttPass.isEmpty() ? nullptr : cfg.mqttPass.c_str(),
                         topic("avail").c_str(), 0, true, "offline");
  if (!ok) return;
  Serial.println("MQTT connected");
  mqtt.publish(topic("avail").c_str(), "online", true);
  mqtt.subscribe(topic("set").c_str());
  mqtt.subscribe("homeassistant/status");
  publishDiscovery();
  publishState();
}

// ===== Веб =====
void sendState() {
  JsonDocument d;
  d["power"]    = power;
  d["mode"]     = mode;
  d["animMode"] = M_ANIM;
  d["bri"]      = brightness;
  char hex[7];
  snprintf(hex, sizeof(hex), "%06X", (unsigned)color);
  d["color"] = hex;
  d["anim"]  = animName;
  JsonArray m = d["modes"].to<JsonArray>();
  for (auto n : MODE_NAMES) m.add(n);
  JsonArray a = d["anims"].to<JsonArray>();
  for (auto& x : anims) {
    JsonObject o = a.add<JsonObject>();
    o["name"]   = x.name;
    o["frames"] = x.frames;
  }
  d["fsFree"]  = LittleFS.totalBytes() - LittleFS.usedBytes();
  d["fsTotal"] = LittleFS.totalBytes();
  d["ip"]      = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  d["mqtt"]    = cfg.mqttHost.isEmpty() ? "вимкнено" : mqtt.connected() ? "підключено" : "немає з'єднання";
  JsonObject c = d["cfg"].to<JsonObject>();
  c["rot"]   = cfg.rot;
  c["flip"]  = cfg.flip;
  c["serp"]  = cfg.serp;
  c["maxma"] = cfg.maxMa;
  c["ssid"]  = cfg.ssid;
  c["mhost"] = cfg.mqttHost;
  c["mport"] = cfg.mqttPort;
  c["muser"] = cfg.mqttUser;
  String s;
  serializeJson(d, s);
  server.send(200, "application/json", s);
}

void handleSet() {
  if (server.hasArg("power")) power = server.arg("power").toInt();
  if (server.hasArg("mode"))  setMode(server.arg("mode").toInt());
  if (server.hasArg("anim"))  playAnim(server.arg("anim"));
  if (server.hasArg("bri"))   brightness = constrain(server.arg("bri").toInt(), 1, 255);
  if (server.hasArg("color")) setColor(strtoul(server.arg("color").c_str(), nullptr, 16));
  changed();
  sendState();
}

// Діагностика: причина останнього перезапуску й підсумок coredump останнього падіння (адреси — для addr2line)
void handleCrash() {
  String s = "reset=" + String(esp_reset_reason()) + "\n";
  esp_core_dump_summary_t sum;
  if (esp_core_dump_get_summary(&sum) == ESP_OK) {
    s += "task=" + String(sum.exc_task) + " pc=0x" + String(sum.exc_pc, HEX) + "\nbt=";
    for (uint32_t i = 0; i < sum.exc_bt_info.depth; i++) s += "0x" + String(sum.exc_bt_info.bt[i], HEX) + " ";
  } else s += "no coredump";
  server.send(200, "text/plain", s);
}

void handleCfg() {
  if (server.hasArg("rot"))   cfg.rot   = server.arg("rot").toInt() % 4;
  if (server.hasArg("flip"))  cfg.flip  = server.arg("flip").toInt();
  if (server.hasArg("serp"))  cfg.serp  = server.arg("serp").toInt();
  if (server.hasArg("maxma")) cfg.maxMa = constrain(server.arg("maxma").toInt(), 300, 20000);
  if (server.hasArg("rot") || server.hasArg("flip") || server.hasArg("serp")) testUntil = millis() + 5000;
  buildMap();
  saveConfig();
  sendState();
}

void handleNet() {
  if (server.hasArg("ssid"))  cfg.ssid     = server.arg("ssid");
  if (server.arg("wpass").length()) cfg.pass = server.arg("wpass");
  if (server.hasArg("mhost")) cfg.mqttHost = server.arg("mhost");
  if (server.hasArg("mport")) cfg.mqttPort = server.arg("mport").toInt() ?: 1883;
  if (server.hasArg("muser")) cfg.mqttUser = server.arg("muser");
  if (server.arg("mpass").length()) cfg.mqttPass = server.arg("mpass");
  cfg.mqttHost.trim();
  saveConfig();
  server.send(200, "text/plain", "OK");
  delay(500);
  ESP.restart();
}

void handleDel() {
  String n = cleanName(server.arg("name"));
  if (mode == M_ANIM && animName == n) setMode(M_SOLID);
  LittleFS.remove(animPath(n));
  scanAnims();
  changed();
  publishDiscovery();
  sendState();
}

// ===== HTTPS: трансляція з камери =====
volatile bool camFrame = false;           // прийшов кадр із камери (обробляється в loop)

static esp_err_t camPageHandler(httpd_req_t* req) {
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  return httpd_resp_send(req, CAM_PAGE, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t camWsHandler(httpd_req_t* req) {
  if (req->method == HTTP_GET) return ESP_OK;             // рукостискання WebSocket
  static uint8_t buf[FRAME_BYTES];
  httpd_ws_frame_t f = {};
  esp_err_t r = httpd_ws_recv_frame(req, &f, 0);          // спершу лише довжина
  if (r != ESP_OK) return r;
  if (f.len > sizeof(buf)) return ESP_FAIL;
  f.payload = buf;
  r = httpd_ws_recv_frame(req, &f, sizeof(buf));
  if (r != ESP_OK) return r;
  if (f.type == HTTPD_WS_TYPE_BINARY && f.len == FRAME_BYTES) {
    memcpy(drawBuf, buf, FRAME_BYTES);
    camFrame = true;
  }
  return ESP_OK;
}

void startHttps() {
  httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();
  conf.servercert     = (const uint8_t*)CERT_PEM;
  conf.servercert_len = sizeof(CERT_PEM);
  conf.prvtkey_pem    = (const uint8_t*)KEY_PEM;
  conf.prvtkey_len    = sizeof(KEY_PEM);
  conf.httpd.max_open_sockets = 3;
  conf.httpd.lru_purge_enable = true;
  httpd_handle_t srv = nullptr;
  if (httpd_ssl_start(&srv, &conf) != ESP_OK) { Serial.println("HTTPS start failed"); return; }
  httpd_uri_t page = {};
  page.uri = "/"; page.method = HTTP_GET; page.handler = camPageHandler;
  httpd_register_uri_handler(srv, &page);
  httpd_uri_t ws = {};
  ws.uri = "/ws"; ws.method = HTTP_GET; ws.handler = camWsHandler; ws.is_websocket = true;
  httpd_register_uri_handler(srv, &ws);
}

// Живе малювання: px = 16*16*3 байт у hex, рядками зверху
void handleDraw() {
  const String& hx = server.arg("px");
  if (hx.length() != FRAME_BYTES * 2) { server.send(400, "text/plain", "bad frame"); return; }
  auto nib = [](char c) -> uint8_t { return c <= '9' ? c - '0' : (c | 0x20) - 'a' + 10; };
  for (int i = 0; i < FRAME_BYTES; i++) drawBuf[i] = nib(hx[2 * i]) << 4 | nib(hx[2 * i + 1]);
  bool wasDraw = mode == M_DRAW && power;
  closeAnim();
  mode  = M_DRAW;
  power = true;
  if (!wasDraw) publishState();
  server.send(204);
}

void handleFile() {
  File f = LittleFS.open(animPath(cleanName(server.arg("name"))), "r");
  if (!f) { server.send(404, "text/plain", "not found"); return; }
  server.streamFile(f, "application/octet-stream");
  f.close();
}

File   upFile;
String upName;
bool   upOk = false;

void handleUploadChunk() {
  HTTPUpload& u = server.upload();
  if (u.status == UPLOAD_FILE_START) {
    upName = cleanName(server.arg("name"));
    upOk = upName.length() > 0 && upName.length() <= 48;
    if (!upOk) return;
    if (animName == upName) closeAnim();
    upFile = LittleFS.open(animPath(upName), "w");
    upOk = (bool)upFile;
  } else if (u.status == UPLOAD_FILE_WRITE) {
    if (upOk && upFile.write(u.buf, u.currentSize) != u.currentSize) upOk = false;
  } else if (u.status == UPLOAD_FILE_END || u.status == UPLOAD_FILE_ABORTED) {
    if (upFile) upFile.close();
    if (u.status == UPLOAD_FILE_ABORTED) upOk = false;
    if (!upOk && upName.length() && upName.length() <= 48) LittleFS.remove(animPath(upName));
  }
}

void handleUploadDone() {
  scanAnims();
  if (!upOk) {
    if (mode == M_ANIM && !openAnim()) setMode(M_SOLID);
    server.send(507, "text/plain", "Не вдалося зберегти: не вистачає місця або неправильна назва");
    return;
  }
  if (!playAnim(upName)) {
    LittleFS.remove(animPath(upName));
    scanAnims();
    server.send(400, "text/plain", "Файл пошкоджений");
    return;
  }
  power = true;
  changed();
  publishDiscovery();
  sendState();
}

void handleOtaChunk() {
  HTTPUpload& u = server.upload();
  if (u.status == UPLOAD_FILE_START)        { closeAnim(); Update.begin(UPDATE_SIZE_UNKNOWN); }
  else if (u.status == UPLOAD_FILE_WRITE)   Update.write(u.buf, u.currentSize);
  else if (u.status == UPLOAD_FILE_END)     Update.end(true);
  else if (u.status == UPLOAD_FILE_ABORTED) Update.abort();
}

void handleOtaDone() {
  bool ok = !Update.hasError();
  server.send(ok ? 200 : 500, "text/plain", ok ? "OK" : Update.errorString());
  if (ok) { delay(500); ESP.restart(); }
}

// ===== Wi-Fi =====
void setupWiFi() {
  WiFi.setHostname(HOSTNAME);
  WiFi.mode(WIFI_STA);
  if (cfg.ssid.length()) {
    WiFi.begin(cfg.ssid.c_str(), cfg.pass.c_str());
    for (int i = 0; i < 60 && WiFi.status() != WL_CONNECTED; i++) delay(250);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("WiFi OK, IP: %s\n", WiFi.localIP().toString().c_str());
    return;
  }
  // Мережі нема: піднімаємо свою точку доступу, але далі пробуємо підключитися
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS);
  apMode = true;
  Serial.printf("AP '%s', IP: %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
}

// ===== Основне =====
void setup() {
  Serial.begin(115200);
  loadAll();
  buildMap();
  for (int i = 0; i < 256; i++) gamma8[i] = (uint8_t)(powf(i / 255.0f, 2.0f) * 255.0f + 0.5f);

  strip.begin();
  strip.clear();
  strip.show();

  if (!LittleFS.begin(true)) Serial.println("LittleFS error");
  LittleFS.mkdir("/a");
  scanAnims();
  if (mode == M_ANIM) playAnim(animName);
  else if (mode >= MODE_COUNT) mode = M_SOLID;

  char id[16];
  snprintf(id, sizeof(id), "lamp_%06x", (unsigned)((ESP.getEfuseMac() >> 24) & 0xFFFFFF));
  devId = id;

  setupWiFi();
  MDNS.begin(HOSTNAME);
  MDNS.addService("http", "tcp", 80);

  mqtt.setBufferSize(4096);
  mqtt.setSocketTimeout(3);
  mqtt.setCallback(onMqtt);

  server.on("/",       [] { server.send(200, "text/html", PAGE); });
  server.on("/state",  sendState);
  server.on("/crash",  handleCrash);
  server.on("/set",    handleSet);
  server.on("/cfg",    handleCfg);
  server.on("/net",    HTTP_POST, handleNet);
  server.on("/del",    handleDel);
  server.on("/draw",   HTTP_POST, handleDraw);
  server.on("/file",   handleFile);
  server.on("/upload", HTTP_POST, handleUploadDone, handleUploadChunk);
  server.on("/update", HTTP_POST, handleOtaDone, handleOtaChunk);
  server.begin();
  startHttps();
  Serial.printf("Lamp ready, id %s\n", devId.c_str());
}

void loop() {
  server.handleClient();
  mqttLoop();

  if (apMode && WiFi.status() == WL_CONNECTED) {          // домашня мережа з'явилась
    WiFi.softAPdisconnect(true);
    apMode = false;
    Serial.printf("WiFi OK, IP: %s\n", WiFi.localIP().toString().c_str());
  }

  if (camFrame) {                                          // кадр із камери: показуємо як живий малюнок
    camFrame = false;
    if (mode != M_DRAW || !power) { closeAnim(); mode = M_DRAW; power = true; publishState(); }
  }

  if (dirtyAt && millis() - dirtyAt > 2000) { saveState(); dirtyAt = 0; }

  static uint32_t last = 0;
  if (millis() - last >= 20) {
    last = millis();
    static bool wasOn = false;
    if (power && !wasOn) fxStart = true;                  // після вмикання ефект починається спочатку (Світанок, Захід)
    wasOn = power;
    if ((int32_t)(testUntil - millis()) > 0) drawTest();
    else if (power) {
      if (mode == M_ANIM) animStep();
      else if (mode == M_DRAW) drawStep();
      else { EFFECTS[mode](); fxStart = false; }
    }
    showLeds();
  }
}
