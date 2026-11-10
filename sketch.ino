
const int SOIL_PIN = A0;
const int TANK_SWITCH_PIN = A1;
const int RELAY_PIN = 8;
const int BUZZER_PIN = 9;
const int GREEN_LED_PIN = 10;
const int RED_LED_PIN = 11;

const bool RELAY_ACTIVE_LOW = true;

const int SOIL_DRY_THRESHOLD = 550;
const int SOIL_WET_THRESHOLD = 500;
const int SOIL_FAULT_LOW = 5;

const unsigned long MAX_WATER_MS = 10000;
const unsigned long REWATER_DELAY_MS = 5000;
const unsigned long SAMPLE_MS = 1000;
const unsigned long BEEP_MS = 300;

const int MELODY[] = {
  659, 622, 659, 622, 659, 494, 587, 523, 440, 0,
  262, 330, 440, 494, 0,
  330, 415, 494, 523, 0
};

const int NOTE_TIME[] = {
  200, 200, 200, 200, 200, 200, 200, 200, 600, 300,
  200, 200, 200, 600, 300,
  200, 200, 200, 600, 300
};

const int MELODY_LENGTH = 19;

bool soilDry = false;
bool tankHasWater = false;
bool soilFault = false;
bool pumpRunning = false;
bool hasWatered = false;
bool alarmActive = false;

int soilRaw = 0;

unsigned long pumpStart = 0;
unsigned long lastWatered = 0;
unsigned long lastSample = 0;
unsigned long lastStatus = 0;
unsigned long lastBeep = 0;
unsigned long noteStart = 0;

int melodyIndex = 0;
bool beepState = false;

void motor(bool state) {

  if (pumpRunning == state) return;

  digitalWrite(RELAY_PIN,
    RELAY_ACTIVE_LOW ? (state ? LOW : HIGH) : (state ? HIGH : LOW));

  pumpRunning = state;

  if (state) {
    pumpStart = millis();
    Serial.println("MOTOR ON");
  } else {
    Serial.println("MOTOR OFF");
  }
}

void stopAlarm() {
  noTone(BUZZER_PIN);
  alarmActive = false;
  beepState = false;
}

void updateAlarm(unsigned long now) {

  if (!alarmActive) {
    alarmActive = true;
    melodyIndex = 0;
    noteStart = now;
    tone(BUZZER_PIN, MELODY[0]);
  }

  if (now - noteStart >= NOTE_TIME[melodyIndex]) {
    melodyIndex = (melodyIndex + 1) % MELODY_LENGTH;
    noteStart = now;

    if (MELODY[melodyIndex] == 0) {
      noTone(BUZZER_PIN);
    } else {
      tone(BUZZER_PIN, MELODY[melodyIndex]);
    }
  }
}

void readSensors() {

  soilRaw = analogRead(SOIL_PIN);

  tankHasWater = (digitalRead(TANK_SWITCH_PIN) == LOW);

  soilFault = (soilRaw <= SOIL_FAULT_LOW);

  if (soilRaw >= SOIL_DRY_THRESHOLD) {
    soilDry = true;
  }

  if (soilRaw <= SOIL_WET_THRESHOLD) {
    soilDry = false;
  }
}

void setup() {

  Serial.begin(9600);

  pinMode(SOIL_PIN, INPUT);
  pinMode(TANK_SWITCH_PIN, INPUT_PULLUP);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);

  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  noTone(BUZZER_PIN);

  readSensors();

  lastSample = millis();
  lastStatus = millis();

  Serial.println("WATERING SYSTEM STARTED");
}

void loop() {

  unsigned long now = millis();

  if (now - lastSample >= SAMPLE_MS) {
    lastSample = now;
    readSensors();
  }

  if (!tankHasWater) {

    motor(false);

    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, HIGH);

    updateAlarm(now);

  } else {

    stopAlarm();

    digitalWrite(GREEN_LED_PIN, HIGH);
    digitalWrite(RED_LED_PIN, LOW);

    if (soilFault || !soilDry) {

      motor(false);
      hasWatered = false;

    } else {

      if (pumpRunning) {

        if (now - pumpStart >= MAX_WATER_MS) {
          motor(false);
          lastWatered = now;
          hasWatered = true;
        }

      } else if (!hasWatered ||
                 now - lastWatered >= REWATER_DELAY_MS) {

        motor(true);
      }
    }
  }

  if (now - lastStatus >= 1000) {

    lastStatus = now;

    Serial.print("Soil Value: ");
    Serial.print(soilRaw);

    Serial.print(" | Soil: ");
    Serial.print(soilFault ? "FAULT" : soilDry ? "DRY" : "WET");

    Serial.print(" | Tank: ");
    Serial.print(tankHasWater ? "FULL" : "EMPTY");

    Serial.print(" | Motor: ");
    Serial.println(pumpRunning ? "ON" : "OFF");
  }
}
