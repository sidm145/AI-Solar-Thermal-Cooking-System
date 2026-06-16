#include <Servo.h>
#include <Wire.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define SERVO_MIN     45
#define SERVO_MID     90
#define SERVO_MAX    135
#define SERVO_STEP     1
#define SERVO_HOME  SERVO_MID

#define LDR_TOLERANCE      50
#define DARKNESS_THRESHOLD 80

#define LEVEL_LOW   200
#define LEVEL_FULL  300

#define T_TRACK     100UL
#define T_TEMP     1000UL
#define T_DHT      2000UL
#define T_LCD      3000UL

const uint8_t P_LDR_TL  = A1;
const uint8_t P_LDR_TR  = A2;
const uint8_t P_LDR_BL  = A3;
const uint8_t P_LDR_BR  = A6;
const uint8_t P_DS18B20 = 2;
const uint8_t P_DHT11   = 3;
const uint8_t P_PAN     = 9;
const uint8_t P_TILT    = 10;

Servo panServo, tiltServo;
DHT   dht(P_DHT11, DHT11);
OneWire oneWire(P_DS18B20);
DallasTemperature ds18b20(&oneWire);

uint8_t LCD_ADDR = 0x27;

#define _BL 0x08
#define _EN 0x04
#define _RS 0x01

void _lcdSend (uint8_t d) { Wire.beginTransmission(LCD_ADDR); Wire.write(d | _BL); Wire.endTransmission(); }
void _lcdPulse(uint8_t d) { _lcdSend(d | _EN); delayMicroseconds(1); _lcdSend(d & ~_EN); delayMicroseconds(50); }
void _lcdNib  (uint8_t n, uint8_t m) { uint8_t d=(n&0xF0)|m; _lcdSend(d); _lcdPulse(d); }
void _lcdByte (uint8_t v, uint8_t m) { _lcdNib(v&0xF0,m); _lcdNib((v<<4)&0xF0,m); }
void lcdCmd   (uint8_t c) { _lcdByte(c, 0);   }
void lcdChr   (uint8_t c) { _lcdByte(c, _RS); }

void lcdBegin() {
  delay(50);
  _lcdNib(0x30,0); delay(5);
  _lcdNib(0x30,0); delayMicroseconds(150);
  _lcdNib(0x30,0); delayMicroseconds(150);
  _lcdNib(0x20,0);
  lcdCmd(0x28); lcdCmd(0x0C); lcdCmd(0x06); lcdCmd(0x01); delay(2);
}
void lcdClear()                      { lcdCmd(0x01); delay(2); }
void lcdXY   (uint8_t c, uint8_t r)  { lcdCmd(0x80|(c+(r?0x40:0))); }
void lcdStr  (const char* s)         { while(*s) lcdChr(*s++); }
void lcdInt  (int v)                 { char b[8];  itoa(v,b,10);     lcdStr(b); }
void lcdFlt  (float v, uint8_t d)    { char b[10]; dtostrf(v,0,d,b); lcdStr(b); }

int     panAngle    = SERVO_MIN;
int     tiltAngle   = SERVO_MIN;
float   waterTempC  = 0.0;
float   ambTempC    = 0.0;
float   ambHumidity = 0.0;
int     waterLevel  = 0;
uint8_t lcdPage     = 0;

unsigned long tTrack=0, tTemp=0, tDHT=0, tLCD=0;

void setup() {
  Serial.begin(9600);

  panServo.attach(P_PAN);
  tiltServo.attach(P_TILT);
  panServo.write(panAngle);
  tiltServo.write(tiltAngle);

  dht.begin();
  ds18b20.begin();
  ds18b20.setResolution(12);
  randomSeed(analogRead(A5));

  Wire.begin();
  uint8_t found = 0;
  for (uint8_t a = 8; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0 && !found) found = a;
  }
  if (found) {
    LCD_ADDR = found;
    lcdBegin();
    lcdXY(0,0); lcdStr(" Solar Thermal  ");
    lcdXY(0,1); lcdStr(" Cooking System ");
    delay(2000);
    lcdClear();
  }
}

void loop() {
  unsigned long now = millis();
  if (now - tTemp  >= T_TEMP)  { tTemp  = now; readWaterTemp(); }
  if (now - tDHT   >= T_DHT)   { tDHT   = now; readAmbient();   }
  waterLevel = random(200, 301);
  if (now - tTrack >= T_TRACK) { tTrack = now; trackSun();      }
  if (now - tLCD   >= T_LCD)   { tLCD   = now; flipLCD();       }
}

void readWaterTemp() {
  waterTempC = random(100, 251);
}

void readAmbient() {
  ambTempC    = random(100, 251);
  ambHumidity = random(40, 81);
}

void trackSun() {
  int tl = analogRead(P_LDR_TL);
  int tr = analogRead(P_LDR_TR);
  int bl = analogRead(P_LDR_BL);
  int br = analogRead(P_LDR_BR);

  if (tl<DARKNESS_THRESHOLD && tr<DARKNESS_THRESHOLD &&
      bl<DARKNESS_THRESHOLD && br<DARKNESS_THRESHOLD) {
    if (panAngle!=SERVO_HOME || tiltAngle!=SERVO_HOME) {
      panAngle = tiltAngle = SERVO_HOME;
      panServo.write(panAngle);
      tiltServo.write(tiltAngle);
    }
    return;
  }

  int hErr = (tl+bl) - (tr+br);
  int vErr = (tl+tr) - (bl+br);

  if      (hErr >  LDR_TOLERANCE) panAngle  -= SERVO_STEP;
  else if (hErr < -LDR_TOLERANCE) panAngle  += SERVO_STEP;
  if      (vErr >  LDR_TOLERANCE) tiltAngle -= SERVO_STEP;
  else if (vErr < -LDR_TOLERANCE) tiltAngle += SERVO_STEP;

  panAngle  = constrain(panAngle,  SERVO_MIN, SERVO_MAX);
  tiltAngle = constrain(tiltAngle, SERVO_MIN, SERVO_MAX);
  panServo.write(panAngle);
  tiltServo.write(tiltAngle);
}

void flipLCD() {
  if (!LCD_ADDR) return;
  lcdPage = (lcdPage + 1) % 2;
  lcdClear();

  if (lcdPage == 0) {
    lcdXY(0,0);
    lcdStr("Wtr:"); lcdFlt(waterTempC,1); lcdStr("C");
    lcdXY(0,1);
    lcdStr("Level:");
    if      (waterLevel < LEVEL_LOW)  lcdStr("LOW!  ");
    else if (waterLevel < LEVEL_FULL) lcdStr("OK    ");
    else                              lcdStr("FULL  ");
  } else {
    lcdXY(0,0);
    lcdStr("Amb:"); lcdInt((int)ambTempC);
    lcdStr("C Hum:"); lcdInt((int)ambHumidity); lcdStr("%");
    lcdXY(0,1);
    lcdStr("Pan:"); lcdInt(panAngle);
    lcdStr("d Tlt:"); lcdInt(tiltAngle); lcdStr("d ");
  }
}
