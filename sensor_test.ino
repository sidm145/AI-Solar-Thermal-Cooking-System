// =============================================================
//  LCD + SENSORS TEST SKETCH
//  Tests: I2C LCD, DS18B20 water temp, DHT11 ambient, water level
//
//  WIRING REMINDER:
//    LCD SDA  → A4  |  LCD SCL  → A5
//    DS18B20  → D2  (+ 4.7k resistor between D2 and 5V)
//    DHT11    → D3
//    Water Lv → A0
//
//  LIBRARIES NEEDED (Library Manager):
//    • DHT sensor library  — Adafruit
//    • OneWire             — Paul Stoffregen
//    • DallasTemperature   — Miles Burton
//    (Wire.h is built-in — no install needed)
//
//  Open Serial Monitor at 9600 baud after uploading.
// =============================================================

#include <Wire.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// --- Pin Definitions ---
#define PIN_DS18B20   2
#define PIN_DHT11     3
#define PIN_WATER_LVL A0

// --- Sensor Objects ---
DHT dht(PIN_DHT11, DHT11);
OneWire oneWire(PIN_DS18B20);
DallasTemperature ds18b20(&oneWire);

// --- I2C LCD Driver (built-in, no library needed) ---
uint8_t LCD_ADDR = 0x27;    // auto-detected in setup()

#define BL  0x08
#define EN  0x04
#define RS  0x01

void lcdWrite(uint8_t d)             { Wire.beginTransmission(LCD_ADDR); Wire.write(d | BL); Wire.endTransmission(); }
void lcdPulse(uint8_t d)             { lcdWrite(d | EN); delayMicroseconds(1); lcdWrite(d & ~EN); delayMicroseconds(50); }
void lcdNibble(uint8_t n, uint8_t m) { uint8_t d = (n & 0xF0) | m; lcdWrite(d); lcdPulse(d); }
void lcdByte(uint8_t v, uint8_t m)   { lcdNibble(v & 0xF0, m); lcdNibble((v << 4) & 0xF0, m); }
void lcdCmd(uint8_t c)               { lcdByte(c, 0); }
void lcdChar(uint8_t c)              { lcdByte(c, RS); }

void lcdInit() {
  delay(50);
  lcdNibble(0x30, 0); delay(5);
  lcdNibble(0x30, 0); delayMicroseconds(150);
  lcdNibble(0x30, 0); delayMicroseconds(150);
  lcdNibble(0x20, 0);
  lcdCmd(0x28); lcdCmd(0x0C); lcdCmd(0x06); lcdCmd(0x01); delay(2);
}
void lcdClear()              { lcdCmd(0x01); delay(2); }
void lcdPos(uint8_t c, uint8_t r) { lcdCmd(0x80 | (c + (r ? 0x40 : 0x00))); }
void lcdStr(const char* s)   { while (*s) lcdChar(*s++); }
void lcdInt(int v)           { char b[8]; itoa(v, b, 10); lcdStr(b); }
void lcdFloat(float v, int d){ char b[10]; dtostrf(v, 0, d, b); lcdStr(b); }

// =============================================================

void setup() {
  Serial.begin(9600);
  Serial.println(F("===== LCD + SENSOR TEST ====="));

  // --- I2C LCD: Auto-detect address ---
  Wire.begin();
  Serial.println(F("\n[LCD] Scanning I2C bus..."));
  uint8_t found = 0;
  for (uint8_t a = 8; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("  Found device at 0x"));
      if (a < 16) Serial.print(F("0"));
      Serial.println(a, HEX);
      if (!found) found = a;
    }
  }
  if (found) {
    LCD_ADDR = found;
    Serial.print(F("  Using 0x")); if (found < 16) Serial.print(F("0")); Serial.println(found, HEX);
    lcdInit();
    lcdPos(0, 0); lcdStr("  Sensor Test   ");
    lcdPos(0, 1); lcdStr("  Starting...   ");
    delay(1500);
    lcdClear();
    Serial.println(F("  LCD: OK"));
  } else {
    Serial.println(F("  WARNING: No I2C device found!"));
    Serial.println(F("  Check: SDA->A4  SCL->A5  VCC->5V  GND->GND"));
    Serial.println(F("  (continuing without LCD)"));
  }

  // --- DS18B20 ---
  ds18b20.begin();
  Serial.print(F("\n[DS18B20] Devices found: "));
  Serial.println(ds18b20.getDeviceCount());
  if (ds18b20.getDeviceCount() == 0)
    Serial.println(F("  WARNING: No DS18B20! Check D2 wiring + 4.7k resistor."));

  // --- DHT11 ---
  dht.begin();
  Serial.println(F("[DHT11] Initialised on D3"));
  Serial.println(F("[Water Level] Sensor on A0"));

  Serial.println(F("\n===== Readings every 2 seconds ====="));
}

// =============================================================

void loop() {
  // --- Read all sensors ---
  ds18b20.requestTemperatures();
  float waterTemp = ds18b20.getTempCByIndex(0);
  float ambTemp   = dht.readTemperature();
  float ambHum    = dht.readHumidity();
  int   level     = analogRead(PIN_WATER_LVL);

  // --- Serial Monitor output ---
  Serial.println(F("-----------------------------"));

  Serial.print(F("Water Temp (DS18B20): "));
  if (waterTemp <= -100.0) Serial.println(F("ERROR — check wiring & 4.7k resistor"));
  else { Serial.print(waterTemp, 1); Serial.println(F(" C")); }

  Serial.print(F("Ambient Temp (DHT11): "));
  if (isnan(ambTemp)) Serial.println(F("ERROR — check D3 wiring"));
  else { Serial.print(ambTemp, 1); Serial.println(F(" C")); }

  Serial.print(F("Humidity     (DHT11): "));
  if (isnan(ambHum)) Serial.println(F("ERROR"));
  else { Serial.print(ambHum, 0); Serial.println(F(" %")); }

  Serial.print(F("Water Level   (A0)  : "));
  Serial.print(level);
  Serial.println(level < 300 ? F("  [LOW]") : F("  [OK]"));

  // --- LCD output ---
  if (LCD_ADDR) {
    // Line 0: Water temp + humidity
    lcdPos(0, 0);
    lcdStr("W:");
    if (waterTemp <= -100.0) lcdStr("ERR ");
    else { lcdFloat(waterTemp, 1); lcdStr("C "); }
    lcdStr("H:");
    if (isnan(ambHum)) lcdStr("ERR");
    else { lcdInt((int)ambHum); lcdStr("% "); }

    // Line 1: Ambient temp + water level
    lcdPos(0, 1);
    lcdStr("A:");
    if (isnan(ambTemp)) lcdStr("ERR ");
    else { lcdInt((int)ambTemp); lcdStr("C  "); }
    lcdStr("Lv:");
    lcdInt(level);
    lcdStr("   ");
  }

  delay(2000);
}
