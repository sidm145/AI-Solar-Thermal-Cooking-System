// =============================================================
//  PUMP TEST SKETCH
//  Relay on D4 — ON 5 sec, OFF 5 sec, repeat forever.
//  No libraries needed. No sensors. No LCD.
//  Upload this, open Serial Monitor at 9600 baud.
// =============================================================

#define RELAY_PIN  4    // Relay IN connected to Arduino D4

// ACTIVE-LOW  = LOW turns pump ON  (most common blue relay modules)
// ACTIVE-HIGH = HIGH turns pump ON (some modules)
// Change RELAY_ON to HIGH if pump doesn't respond.
#define RELAY_ON   LOW
#define RELAY_OFF  HIGH

#define PUMP_ON_TIME   5000   // ms pump stays ON
#define PUMP_OFF_TIME  5000   // ms pump stays OFF

void setup() {
  Serial.begin(9600);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);   // Start with pump OFF
  Serial.println("Pump test started.");
  Serial.println("ON 5 sec -> OFF 5 sec -> repeat");
}

void loop() {
  // Turn pump ON
  digitalWrite(RELAY_PIN, RELAY_ON);
  Serial.println("PUMP ON");
  delay(PUMP_ON_TIME);

  // Turn pump OFF
  digitalWrite(RELAY_PIN, RELAY_OFF);
  Serial.println("PUMP OFF");
  delay(PUMP_OFF_TIME);
}
