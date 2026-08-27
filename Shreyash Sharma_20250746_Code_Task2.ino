#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Defining the Pins
const int PIN_BUTTON = 2;
const int PIN_ECHO   = 8;
const int PIN_TRIG   = 9;
const int PIN_BUZZER = 10;
const int PIN_LED    = 11;
const int PIN_LDR    = A0;

// Setting thresholds
const int LIGHT_THRESHOLD = 512;       // Half of 1023 (Analog range)
const long DISTANCE_THRESHOLD = 100;   // 100 cm
const unsigned long DANGER_LIMIT = 5000; // 5 seconds in ms


enum ShipState {
  OPEN_SEA,
  STORM,
  CHARYBDIS,
  ANCHOR_DROPPED,
  WRECKED
};

ShipState currentState = OPEN_SEA;
ShipState previousState = OPEN_SEA; // Stores state before anchor was dropped

unsigned long dangerStartTime = 0;
unsigned long lastBlinkTime = 0;
bool ledState = false;


 

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  lcd.init();
  lcd.backlight();
  updateLCD("OPEN SEA");
}

void loop() {
  handleButton();

  // Freeze state if the ship is wrecked until simulation is restarted.
  if (currentState == WRECKED) {
    digitalWrite(PIN_LED, LOW);
    noTone(PIN_BUZZER);
    return;
  }

  // Sensors reading data according to parameters set.
  int lightVal = analogRead(PIN_LDR);
  long distanceCm = getDistance();

  bool isStormCondition = (lightVal > LIGHT_THRESHOLD);
  bool isCharybdisCondition = (distanceCm > 0 && distanceCm < DISTANCE_THRESHOLD);

  // Different cases possible i.e. Open sea, Storm, Charybdis, Anchor Dropped and the changes in LEDs/ Buzzer that will happen.
  switch (currentState) {
    case OPEN_SEA:
      digitalWrite(PIN_LED, LOW);
      noTone(PIN_BUZZER);

      if (isStormCondition) {
        currentState = STORM;
        dangerStartTime = millis();
        updateLCD("STORM");
      } else if (isCharybdisCondition) {
        currentState = CHARYBDIS;
        dangerStartTime = millis();
        updateLCD("CHARYBDIS");
      }
      break;

    case STORM:
      // Blink LED every 150ms
      if (millis() - lastBlinkTime >= 150) {
        lastBlinkTime = millis();
        ledState = !ledState;
        digitalWrite(PIN_LED, ledState);
      }

      if (millis() - dangerStartTime >= DANGER_LIMIT) {
        currentState = WRECKED;
        digitalWrite(PIN_LED, LOW);
        updateLCD("WRECKED");
      } else if (!isStormCondition) {
        currentState = OPEN_SEA;
        digitalWrite(PIN_LED, LOW);
        updateLCD("OPEN SEA");
      }
      break;

    case CHARYBDIS:
      tone(PIN_BUZZER, 1000); // 1kHz alarm tone

      // Check 5-second survival limit
      if (millis() - dangerStartTime >= DANGER_LIMIT) {
        currentState = WRECKED;
        noTone(PIN_BUZZER);
        updateLCD("WRECKED");
      } else if (!isCharybdisCondition) {
        currentState = OPEN_SEA;
        noTone(PIN_BUZZER);
        updateLCD("OPEN SEA");
      }
      break;

    case ANCHOR_DROPPED:
      digitalWrite(PIN_LED, LOW);
      noTone(PIN_BUZZER);
      break;
  }
}


 void handleButton() {
  static bool lastBtnState = HIGH;
  bool currentBtnState = digitalRead(PIN_BUTTON);

  // Detecting the exact moment the button is pressed down.
  if (lastBtnState == HIGH && currentBtnState == LOW) {
    if (currentState != WRECKED) {
      if (currentState == ANCHOR_DROPPED) {
        //  Raise Anchor: return to OPEN SEA
        currentState = OPEN_SEA;
        updateLCD("OPEN SEA");
      } else {
        // Anchor Dropped = Shielding ship.
        previousState = currentState;
        currentState = ANCHOR_DROPPED;
        updateLCD("ANCHOR DROPPED");
      }
    }
    // debounce being set to avoid push-button failure
    delay(50); 
  }
  lastBtnState = currentBtnState;
}

long getDistance() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
//  If the echo does not stay at HIGH for 30ms before going to low, the value returns as 0, ending // the loop.
// Return duration is using distance formula to find time taken by sound to return and divided by // 2 so that’s its One way.
  long duration = pulseIn(PIN_ECHO, HIGH, 30000); 
  if (duration == 0) return 999;                 
  return duration * 0.034 / 2;
}

void updateLCD(const char* stateName) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("State:");
  lcd.setCursor(0, 1);
  lcd.print(stateName);
}