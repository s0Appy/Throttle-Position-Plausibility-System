////////// DEBUG ///////////////////
unsigned long lastDebug = 0;
const unsigned long debugInterval = 50;  // 50 ms = 20 Hz

////////// PINS ///////////////////
const int etb = 2;
const int ign = 7;

////////// APPS and TPS VARIABLES //////////
// ---- APPS VOLTAGES FROM ECU ----
const float appsMainVoltageRaw_100 = 4.5;
const float appsMainVoltageRaw_0 = 0.5;
const float appsTrackVoltageRaw_100 = 0.5;
const float appsTrackVoltageRaw_0 = 4.5;

// ---- TPS VOLTAGES FROM ECU ----
const float tpsMainVoltageRaw_100 = 4.5;
const float tpsMainVoltageRaw_0 = 0.5;
const float tpsTrackVoltageRaw_100 = 0.5;
const float tpsTrackVoltageRaw_0 = 4.5;

/////////// FINAL APPS TPS VALUES //////////
const int appsMain_100 = appsMainVoltageRaw_100 / 5 * 1023;
const int appsMain_0 = appsMainVoltageRaw_0 / 5 * 1023;
const int appsTrack_100 = appsTrackVoltageRaw_100 / 5 * 1023;
const int appsTrack_0 = appsTrackVoltageRaw_0 / 5 * 1023;

const int tpsMain_100 = tpsMainVoltageRaw_100 / 5 * 1023;
const int tpsMain_0 = tpsMainVoltageRaw_0 / 5 * 1023;
const int tpsTrack_100 = tpsTrackVoltageRaw_100 / 5 * 1023;
const int tpsTrack_0 = tpsTrackVoltageRaw_0 / 5 * 1023;

//////////////////////////////////////////////
// FAULT STATES
bool appsFaultActive = false;      // APPS main/track > 10% for > 100ms
bool tpsFaultActive = false;       // TPS main/track > 10% for > 100ms
bool appsTpsFaultActive = false;   // APPS-TPS > 10% for > 1000ms

// SHUTDOWN FLAGS
bool etbShutdown = false;          // ETB power cut
bool ignShutdown = false;          // IGN power cut

// FAULT DETECTION TIMERS
unsigned long appsFaultStart = 0;
unsigned long tpsFaultStart = 0;
unsigned long appsTpsFaultStart = 0;

// RECOVERY TIMER
unsigned long recoveryStart = 0;
bool recoveryTimerRunning = false;

// TIMER FLAGS
bool appsTimerRunning = false;
bool tpsTimerRunning = false;
bool appsTpsTimerRunning = false;

// SENSOR READINGS
int tpsMain   = 0;
int tpsTrack  = 0;
int appsMain  = 0;
int appsTrack = 0;

void setup() {
  pinMode(etb, OUTPUT);
  pinMode(ign, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  // Default: all power ON
  digitalWrite(etb, HIGH);
  digitalWrite(ign, HIGH);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200);
  delay(200);
  printStaticLayout();
}

////////////// PLAUSIBILITY LOGIC ///////////////

void updateFaults(float appsMain_per, float appsTrack_per, 
                  float tpsMain_per, float tpsTrack_per) {
  
  float appsDelta = abs(appsMain_per - appsTrack_per);
  float tpsDelta = abs(tpsMain_per - tpsTrack_per);
  float appsTpsDelta = abs(appsMain_per - tpsMain_per);

  // ---- APPS FAULT: > 10% diff for > 100ms → ETB shutdown ----
  bool appsCondition = (appsDelta > 10.0);
  
  if (appsCondition) {
    if (!appsTimerRunning) {
      appsTimerRunning = true;
      appsFaultStart = millis();
    }
    if (millis() - appsFaultStart >= 100) {
      appsFaultActive = true;
      etbShutdown = true;
    }
  } else {
    appsTimerRunning = false;
    appsFaultActive = false;
  }

  // ---- TPS FAULT: > 10% diff for > 100ms → ETB shutdown ----
  bool tpsCondition = (tpsDelta > 10.0);
  
  if (tpsCondition) {
    if (!tpsTimerRunning) {
      tpsTimerRunning = true;
      tpsFaultStart = millis();
    }
    if (millis() - tpsFaultStart >= 100) {
      tpsFaultActive = true;
      etbShutdown = true;
    }
  } else {
    tpsTimerRunning = false;
    tpsFaultActive = false;
  }

  // ---- APPS-TPS FAULT: > 10% diff for > 1 second → IGN shutdown ----
  bool appsTpsCondition = (appsTpsDelta > 10.0);
  
  if (appsTpsCondition) {
    if (!appsTpsTimerRunning) {
      appsTpsTimerRunning = true;
      appsTpsFaultStart = millis();
    }
    
    unsigned long elapsed = millis() - appsTpsFaultStart;
    
    // Immediate ETB shutdown when fault detected
    if (elapsed >= 0) {
      etbShutdown = true;
      appsTpsFaultActive = true;
    }
    
    // IGN shutdown after 1 second
    if (elapsed >= 1000) {
      ignShutdown = true;
    }
  } else {
    appsTpsTimerRunning = false;
    appsTpsFaultActive = false;
  }

  // ---- RECOVERY LOGIC ----
  // If any shutdown is active, check if TPS is at default position
  if (etbShutdown || ignShutdown) {
    float avgTps = (tpsMain_per + tpsTrack_per) / 2.0;
    
    // TPS must be at or below 10% (unpowered default position)
    if (avgTps <= 10.0 && !appsFaultActive && !tpsFaultActive && !appsTpsFaultActive) {
      if (!recoveryTimerRunning) {
        recoveryTimerRunning = true;
        recoveryStart = millis();
      }
      
      // After 1 second at default position with no faults, allow recovery
      if (millis() - recoveryStart >= 1000) {
        etbShutdown = false;
        ignShutdown = false;
        recoveryTimerRunning = false;
      }
    } else {
      // Reset recovery timer if conditions not met
      recoveryTimerRunning = false;
    }
  } else {
    // No shutdown active, reset recovery timer
    recoveryTimerRunning = false;
  }
}

////////// OUTPUT CONTROL ////////////
void updateOutputs() {
  // ETB control: LOW if shutdown active
  if (etbShutdown) {
    digitalWrite(etb, LOW);
  } else {
    digitalWrite(etb, HIGH);
  }

  // IGN control: LOW if IGN shutdown active
  if (ignShutdown) {
    digitalWrite(ign, LOW);
  } else {
    digitalWrite(ign, HIGH);
  }
}

///////////////// UPDATE STATUS LED ///////////////
// LED indicates overall system state
// - OFF: no faults, normal operation
// - Slow blink: fault detected, timer running
// - Solid: ETB shutdown active
// - Fast blink: IGN shutdown active (full power cut)

void updateLED() {
  static unsigned long lastBlink = 0;

  if (ignShutdown) {
    // Fast blink for severe fault (IGN cut)
    if (millis() - lastBlink > 100) {
      lastBlink = millis();
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
  } else if (etbShutdown) {
    // Solid ON for ETB shutdown
    digitalWrite(LED_BUILTIN, HIGH);
  } else if (appsTimerRunning || tpsTimerRunning || appsTpsTimerRunning) {
    // Slow blink for early fault detection
    if (millis() - lastBlink > 250) {
      lastBlink = millis();
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
  } else {
    // No fault
    digitalWrite(LED_BUILTIN, LOW);
  }
}

///////////////// DEBUG FUNCTIONS //////////////////
void clearScreen() {
  Serial.print("\033[2J");
  Serial.print("\033[H");
}

void printStaticLayout() {
  clearScreen();
  Serial.println("APPS Main V:     ");
  Serial.println("APPS Track V:    ");
  Serial.println("TPS Main V:      ");
  Serial.println("TPS Track V:     ");
  Serial.println("APPS Main %:     ");
  Serial.println("APPS Track %:    ");
  Serial.println("TPS Main %:      ");
  Serial.println("TPS Track %:     ");
  Serial.println("Delta APPS:      ");
  Serial.println("Delta TPS:       ");
  Serial.println("Delta APPS-TPS:  ");
  Serial.println("APPS Fault:      ");
  Serial.println("TPS Fault:       ");
  Serial.println("APPS-TPS Fault:  ");
  Serial.println("ETB Shutdown:    ");
  Serial.println("IGN Shutdown:    ");
  Serial.println("Recovery Timer:  ");
}

void updateValue(int row, float value, int decimals = 0) {
  Serial.print("\033[");
  Serial.print(row);
  Serial.print(";20H");
  Serial.print("        ");
  Serial.print("\033[");
  Serial.print(row);
  Serial.print(";20H");
  Serial.print(value, decimals);
}

void updateText(int row, const char* text) {
  Serial.print("\033[");
  Serial.print(row);
  Serial.print(";20H");
  Serial.print("        ");
  Serial.print("\033[");
  Serial.print(row);
  Serial.print(";20H");
  Serial.print(text);
}

///////////////// MAIN LOOP //////////////
void loop() {
  // Poll sensors
  appsMain = analogRead(A6);
  appsTrack = analogRead(A7);
  tpsMain = analogRead(A4);
  tpsTrack = analogRead(A5);

  // Convert to percentages
  float tpsMain_per = (tpsMain - tpsMain_0) * 100.0 / (tpsMain_100 - tpsMain_0);
  float tpsTrack_per = (tpsTrack - tpsTrack_0) * 100.0 / (tpsTrack_100 - tpsTrack_0);
  float appsMain_per = (appsMain - appsMain_0) * 100.0 / (appsMain_100 - appsMain_0);
  float appsTrack_per = (appsTrack - appsTrack_0) * 100.0 / (appsTrack_100 - appsTrack_0);

  // Constrain to 0-100%
  tpsMain_per = constrain(tpsMain_per, 0, 100);
  tpsTrack_per = constrain(tpsTrack_per, 0, 100);
  appsMain_per = constrain(appsMain_per, 0, 100);
  appsTrack_per = constrain(appsTrack_per, 0, 100);

  // Update fault logic
  updateFaults(appsMain_per, appsTrack_per, tpsMain_per, tpsTrack_per);
  
  // Update outputs and LED
  updateOutputs();
  updateLED();

  // Debug display
  float appsMainV = appsMain / 1023.0 * 5.0;
  float appsTrackV = appsTrack / 1023.0 * 5.0;
  float tpsMainV = tpsMain / 1023.0 * 5.0;
  float tpsTrackV = tpsTrack / 1023.0 * 5.0;

  float appsDelta = abs(appsMain_per - appsTrack_per);
  float tpsDelta = abs(tpsMain_per - tpsTrack_per);
  float appsTpsDelta = abs(appsMain_per - tpsMain_per);

  updateValue(1, appsMainV, 2);
  updateValue(2, appsTrackV, 2);
  updateValue(3, tpsMainV, 2);
  updateValue(4, tpsTrackV, 2);
  updateValue(5, appsMain_per, 1);
  updateValue(6, appsTrack_per, 1);
  updateValue(7, tpsMain_per, 1);
  updateValue(8, tpsTrack_per, 1);
  updateValue(9, appsDelta, 1);
  updateValue(10, tpsDelta, 1);
  updateValue(11, appsTpsDelta, 1);
  
  updateText(12, appsFaultActive ? "ACTIVE" : "OK");
  updateText(13, tpsFaultActive ? "ACTIVE" : "OK");
  updateText(14, appsTpsFaultActive ? "ACTIVE" : "OK");
  updateText(15, etbShutdown ? "SHUTDOWN" : "OK");
  updateText(16, ignShutdown ? "SHUTDOWN" : "OK");
  updateText(17, recoveryTimerRunning ? "RUNNING" : "IDLE");
}
