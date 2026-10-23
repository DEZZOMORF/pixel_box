#include <Adafruit_NeoPixel.h>

#define DATA_PIN 13
#define NUM_LEDS 256

Adafruit_NeoPixel m(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  m.begin();
  m.setBrightness(30);
}

void loop() {
  static uint8_t step = 0;
  const uint32_t colors[] = {m.Color(255, 0, 0), m.Color(0, 255, 0), m.Color(0, 0, 255)};
  m.fill(colors[step]);
  m.show();
  digitalWrite(2, !digitalRead(2));
  Serial.printf("krok %d\n", step);
  step = (step + 1) % 3;
  delay(1000);
}
