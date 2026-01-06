#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>

/* ================= WIFI ================= */
#define WIFI_SSID "revo"
#define WIFI_PASS "12345678"

/* ================= FIREBASE ================= */
#define DATABASE_URL "https://guardian-medicine-box-default-rtdb.firebaseio.com/"
#define DATABASE_SECRET "HXPlh3m8vUpD6trssKros984crTFKidlJxyVaxw6"

/* ================= OBJECTS ================= */
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;

/* ================= I2C ================= */
#define SDA_PIN 21
#define SCL_PIN 22

/* ================= PINS ================= */
#define REED1 32
#define REED2 33
#define REED3 25
#define REED4 26

#define RED1   14
#define RED2   27
#define RED3   12
#define RED4   13
int reedPins[4] = {REED1, REED2, REED3, REED4};


#define GREEN1 18
#define GREEN2 19
#define GREEN3 23
#define GREEN4 5

#define BUZZER 4

/* ================= REED LOGIC ================= */
#define DRAWER_OPEN   LOW
#define DRAWER_CLOSED HIGH

/* ================= FSM ================= */
enum State {
  IDLE,
  WAIT_CLOSE,
  WRONG_DRAWER,
  TAKEN
};

State state = IDLE;

/* ================= SCHEDULE ================= */
int drawerHour[4];
int drawerMinute[4];
int lastTriggeredMinute[4] = {-1, -1, -1, -1};

/* ================= VARIABLES ================= */
int activeDrawer = -1;
int wrongDrawer = -1;

bool greenBlink = false;
bool greenState = false;

unsigned long greenTimer = 0;
unsigned long buzzerTimer = 0;
unsigned long rtcTimer = 0;
unsigned long statusTimer = 0;
unsigned long msgTimer = 0;
/* ================= MEDICINE DISPLAY CONTROL ================= */
String medNames[6];          // max 6 medicines
int medCount = 0;
int medIndex = 0;

bool showMedicines = false;

unsigned long medRotateTimer = 0;
unsigned long medStartTime = 0;

const unsigned long MED_DURATION = 5UL * 60UL * 1000UL; // 5 minutes
const unsigned long MED_ROTATE_INTERVAL = 1000;         // 1 second

/* ================= DRAWER STATE TRACK ================= */
bool lastDrawerState[4] = {
  DRAWER_OPEN, DRAWER_OPEN, DRAWER_OPEN, DRAWER_OPEN
};

/* ================= LED HELPERS ================= */
void allOff() {
  digitalWrite(RED1, LOW);
  digitalWrite(RED2, LOW);
  digitalWrite(RED3, LOW);
  digitalWrite(RED4, LOW);
  digitalWrite(GREEN1, LOW);
  digitalWrite(GREEN2, LOW);
  digitalWrite(GREEN3, LOW);
  digitalWrite(GREEN4, LOW);
  digitalWrite(BUZZER, LOW);
}

void greenOn(int d) {
  digitalWrite(GREEN1, d == 1);
  digitalWrite(GREEN2, d == 2);
  digitalWrite(GREEN3, d == 3);
  digitalWrite(GREEN4, d == 4);
}

void redOn(int d) {
  digitalWrite(RED1, d == 1);
  digitalWrite(RED2, d == 2);
  digitalWrite(RED3, d == 3);
  digitalWrite(RED4, d == 4);
}

/* ================= EVENT-BASED DRAWER LOG ================= */
void printDrawerEvents() {
  bool cur[4] = {
    digitalRead(REED1),
    digitalRead(REED2),
    digitalRead(REED3),
    digitalRead(REED4)
  };

  for (int i = 0; i < 4; i++) {
    if (cur[i] != lastDrawerState[i]) {
      Serial.print("[DRAWER ");
      Serial.print(i + 1);
      Serial.print("] ");
      Serial.println(cur[i] == DRAWER_OPEN ? "OPEN" : "CLOSED");
      lastDrawerState[i] = cur[i];
    }
  }
}

/* ================= LIVE STATUS (EVERY 1s) ================= */
void printLiveStatus(DateTime now) {
  if (millis() - statusTimer < 1000) return;
  statusTimer = millis();

  Serial.print("[STATUS] ");
  Serial.print("D1:");
  Serial.print(digitalRead(REED1) == DRAWER_OPEN ? "OPEN  " : "CLOSED ");
  Serial.print("D2:");
  Serial.print(digitalRead(REED2) == DRAWER_OPEN ? "OPEN  " : "CLOSED ");
  Serial.print("D3:");
  Serial.print(digitalRead(REED3) == DRAWER_OPEN ? "OPEN  " : "CLOSED ");
  Serial.print("D4:");
  Serial.print(digitalRead(REED4) == DRAWER_OPEN ? "OPEN  " : "CLOSED ");

  Serial.print("| LED: ");
  if (greenBlink) {
    Serial.print("GREEN BLINK (D");
    Serial.print(activeDrawer + 1);
    Serial.print(")");
  } else if (wrongDrawer != -1) {
    Serial.print("RED (D");
    Serial.print(wrongDrawer + 1);
    Serial.print(")");
  } else {
    Serial.print("OFF");
  }

  Serial.print(" | BUZZER: ");
  Serial.println(digitalRead(BUZZER) ? "ON" : "OFF");
}

/* ================= DRAWER CHECK ================= */
bool correctDrawerClosed() {
  if (activeDrawer == 0) return digitalRead(REED1) == DRAWER_CLOSED;
  if (activeDrawer == 1) return digitalRead(REED2) == DRAWER_CLOSED;
  if (activeDrawer == 2) return digitalRead(REED3) == DRAWER_CLOSED;
  if (activeDrawer == 3) return digitalRead(REED4) == DRAWER_CLOSED;
  return false;
}

bool wrongDrawerClosed() {
  if (activeDrawer != 0 && digitalRead(REED1) == DRAWER_CLOSED) { wrongDrawer = 0; return true; }
  if (activeDrawer != 1 && digitalRead(REED2) == DRAWER_CLOSED) { wrongDrawer = 1; return true; }
  if (activeDrawer != 2 && digitalRead(REED3) == DRAWER_CLOSED) { wrongDrawer = 2; return true; }
  if (activeDrawer != 3 && digitalRead(REED4) == DRAWER_CLOSED) { wrongDrawer = 3; return true; }
  return false;
}

/* ================= TIME CHECK ================= */
void checkTime(DateTime now) {
  for (int i = 0; i < 4; i++) {
    if (now.hour() == drawerHour[i] &&
        now.minute() == drawerMinute[i] &&
        lastTriggeredMinute[i] != now.minute()) {

      lastTriggeredMinute[i] = now.minute();
      activeDrawer = i;
      showMedicines = true;          // ✅ ENABLE medicine display
      medStartTime = millis();      // ✅ start 5-minute timer
      medRotateTimer = millis();    // ✅ start 1-second rotation


      Serial.print("[ALARM] Drawer ");
      Serial.print(i + 1);
      Serial.print(" at ");
      Serial.print(now.hour());
      Serial.print(":");
      Serial.println(now.minute());

      lcd.clear();
      lcd.print("open Drawer ");
      lcd.print(i + 1);

      readMedicines(i); 

      greenBlink = true;
      greenTimer = millis();

      digitalWrite(BUZZER, HIGH);
      buzzerTimer = millis();

      state = WAIT_CLOSE;
    }
  }
}

/* ================= FIREBASE READ ================= */
void readSchedule() {
  for (int i = 0; i < 4; i++) {
    Firebase.RTDB.getInt(&fbdo, "/schedule/" + String(i + 1) + "/hour");
    drawerHour[i] = fbdo.intData();
    Firebase.RTDB.getInt(&fbdo, "/schedule/" + String(i + 1) + "/minute");
    drawerMinute[i] = fbdo.intData();
  }
}
void readMedicines(int drawer) {
  medCount = 0;
  medIndex = 0;

  String basePath = "/drawers/" + String(drawer + 1) + "/medicines";

  if (!Firebase.RTDB.getJSON(&fbdo, basePath)) return;

  FirebaseJson &json = fbdo.jsonObject();
  FirebaseJsonData data;

  size_t len = json.iteratorBegin();
  String key, value;
  int type;

  for (size_t i = 0; i < len && medCount < 6; i++) {
    json.iteratorGet(i, type, key, value);

    if (Firebase.RTDB.getString(&fbdo, basePath + "/" + key + "/name")) {
      String name = fbdo.stringData();
      name.toUpperCase();
      medNames[medCount++] = name.substring(0, 3);
    }
  }
  json.iteratorEnd();
}


/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);
  Serial.println("\n[BOOT] ESP32 Started");

  Wire.begin(SDA_PIN, SCL_PIN);
  
  lcd.init();
  lcd.backlight();

  pinMode(REED1, INPUT_PULLUP);
  pinMode(REED2, INPUT_PULLUP);
  pinMode(REED3, INPUT_PULLUP);
  pinMode(REED4, INPUT_PULLUP);

  pinMode(RED1, OUTPUT);
  pinMode(RED2, OUTPUT);
  pinMode(RED3, OUTPUT);
  pinMode(RED4, OUTPUT);
  pinMode(GREEN1, OUTPUT);
  pinMode(GREEN2, OUTPUT);
  pinMode(GREEN3, OUTPUT);
  pinMode(GREEN4, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  allOff();

  Serial.print("[WIFI] Connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\n[WIFI] Connected");

  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("[FIREBASE] Connected");

  rtc.begin();
  configTime(6 * 3600, 0, "pool.ntp.org");
  Serial.println("[RTC] Ready");
}

/* ================= LOOP ================= */
void loop() {
  DateTime now = rtc.now();

/* ================= MEDICINE NAME DISPLAY ================= */
if (showMedicines && state == WAIT_CLOSE) {

  // stop after 5 minutes
  if (millis() - medStartTime >= MED_DURATION) {
    showMedicines = false;
    lcd.setCursor(0, 1);
    lcd.print("                "); // clear line
  }

  // rotate every 1 second
  if (millis() - medRotateTimer >= MED_ROTATE_INTERVAL && medCount > 0) {
    medRotateTimer = millis();

    lcd.setCursor(0, 1);
    lcd.print("   "); // clear previous text
    lcd.setCursor(0, 1);
    lcd.print(medNames[medIndex]);

    medIndex++;
    if (medIndex >= medCount) medIndex = 0;
  }
}

  printDrawerEvents();     // event-based
  printLiveStatus(now);    // live snapshot

  if (millis() - rtcTimer >= 1000) {
    rtcTimer = millis();
    Serial.print("[TIME] ");
    Serial.print(now.hour());
    Serial.print(":");
    Serial.println(now.minute());
  }

  readSchedule();

  /* Green blink */
  if (greenBlink && millis() - greenTimer >= 500) {
    greenTimer = millis();
    greenState = !greenState;
    greenState ? greenOn(activeDrawer + 1) : allOff();
  }

  /* Reminder buzzer auto-off */
  if (millis() - buzzerTimer >= 2000) {
    digitalWrite(BUZZER, LOW);
  }

  switch (state) {
    case IDLE:
      checkTime(now);
      break;

    case WAIT_CLOSE:
      if (wrongDrawerClosed()) {
        Serial.print("[ERROR] Wrong drawer opened: ");
        Serial.println(wrongDrawer + 1);
        greenBlink = false;
        redOn(wrongDrawer + 1);
        digitalWrite(BUZZER, HIGH);
        lcd.clear();
        lcd.print("WRONG DRAWER");
        state = WRONG_DRAWER;
      } else if (correctDrawerClosed()) {
        showMedicines = false;   // ✅ STOP medicine display

        Serial.println("[TAKEN] Medicine taken");
        greenBlink = false;
        allOff();
        lcd.clear();
        lcd.print("Medicine Taken");
        msgTimer = millis();
        state = TAKEN;
      }
      break;
      case WRONG_DRAWER:
        if (digitalRead(reedPins[wrongDrawer]) == DRAWER_OPEN) {
            Serial.println("[INFO] Wrong drawer reopened");
            allOff();
            wrongDrawer = -1;
          greenBlink = true;
          state = WAIT_CLOSE;
          }
        break;


    case TAKEN:
      if (millis() - msgTimer >= 1500) {
        lcd.clear();
        state = IDLE;
      }
      break;
  }
}
