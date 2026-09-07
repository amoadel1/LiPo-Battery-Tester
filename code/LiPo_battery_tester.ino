#include <Wire.h>
#include "tla2528.h"


// ============================================================
// LiPo BATTERY TESTER
// Maker UNO + TLA2528 + I2C LCD
// ============================================================
//
// Hardware:
//   Maker UNO
//   TLA2528 ADC       -> I2C address 0x10
//   LCD               -> I2C address 0x27
//   Button             -> D7
//   Red LED            -> D4
//   Buzzer             -> D8
//
// TLA2528 channels:
//   CH0 -> Cell 1 cumulative voltage
//   CH1 -> Cell 1 + Cell 2 cumulative voltage
//   CH2 -> Cell 1 + Cell 2 + Cell 3 cumulative voltage
//   CH3 -> Separate PACK connector
//
// Battery types:
//   1S, 2S, 3S
//
// Button operation:
//   Selection screen:
//       Short press -> change 1S / 2S / 3S
//       Long press  -> confirm selection
//
//   Ready screen:
//       Short press -> start test
//       Long press  -> return to battery selection
//
//   Result screen:
//       Short press -> no action
//       Long press  -> return to battery selection
//
// CELL RESULT:
//       <= 3.70 V -> CHG
//       > 3.70 V and <= 4.20 V -> Normal
//       > 4.20 V -> FAIL
//
// PASS:
//       All active cells are normal
//       AND PACK must pass
//
// CHG:
//       One or more cells need charging.
//
// FAIL:
//       One or more cells are above 4.20 V
//       OR PACK voltage does not match.
//
// LED:
//       PASS -> ON continuously
//       CHG  -> Blinking
//       FAIL -> Blinking
//
// BUZZER:
//       PASS -> 1 beep
//       CHG  -> 3 beeps
//       FAIL -> 3 beeps
//
// The result remains on the LCD until:
//       1. Battery is removed, OR
//       2. Long press is detected
//
// ============================================================


// ============================================================
// I2C ADDRESSES
// ============================================================

#define TLA_ADDRESS 0x10
#define LCD_ADDRESS 0x27

TLA2528 adc(TLA_ADDRESS);


// ============================================================
// HARDWARE PINS
// ============================================================

#define BUTTON_PIN 7
#define LED_PIN    4
#define BUZZER_PIN 8


// ============================================================
// BATTERY TYPES
// ============================================================

#define BATTERY_1S 1
#define BATTERY_2S 2
#define BATTERY_3S 3

// Default battery selection when the tester starts
int selectedBattery = BATTERY_3S;


// ============================================================
// BUTTON TIMING
// ============================================================

// Long press is triggered after 1.5 seconds.
// The user does NOT need to release the button.
const unsigned long LONG_PRESS_TIME = 1500;


// ============================================================
// BATTERY CELL VOLTAGE LIMITS
// ============================================================
//
// <= 3.70 V -> CHG
//
// > 3.70 V and <= 4.20 V -> Normal
//
// > 4.20 V -> FAIL
//
// IMPORTANT:
// The displayed voltage is rounded to two decimal places.
// A displayed value of 3.70 V is therefore treated as CHG.
//

const float CELL_CHARGE_MIN_V = 3.70;

const float CELL_MAX_V = 4.20;


// ============================================================
// PACK VOLTAGE MATCH TOLERANCE
// ============================================================
//
// CH3 reads the separate PACK connector.
//
// The PACK voltage is compared with the cumulative cell voltage:
//
// 1S -> CH0
// 2S -> CH1
// 3S -> CH2
//
// Maximum allowed difference:
//     0.25 V
//

const float PACK_MATCH_TOLERANCE = 0.25;


// ============================================================
// TLA2528 VOLTAGE DIVIDER VALUES
// ============================================================
//
// These values are from the existing/old TLA hardware.
// The resistor dividers have NOT been changed.
//

const float DIVIDER_CH0 = 2.0;

const float DIVIDER_CH1 = 3.0;

const float DIVIDER_CH2 = 4.076923;

// CH3 uses the same divider as CH2.
const float DIVIDER_CH3 = DIVIDER_CH2;


// ============================================================
// TLA2528 CALIBRATION VALUES
// ============================================================
//
// Existing calibration values from the previous tester.
//

const float CAL_CH0 = 0.98875;

const float CAL_CH1 = 1.00316;

const float CAL_CH2 = 1.00988;

// CH3 uses the same calibration as CH2.
const float CAL_CH3 = CAL_CH2;


// ============================================================
// BUTTON STATE VARIABLES
// ============================================================

bool buttonPrevious = HIGH;

bool buttonPressed = false;

bool longPressTriggered = false;

unsigned long buttonPressStart = 0;


// ============================================================
// FAIL / CHG LED BLINK VARIABLES
// ============================================================

const unsigned long FAIL_BLINK_INTERVAL = 250;

unsigned long lastFailBlink = 0;

bool failLedState = false;


// ============================================================
// BATTERY SELECTION BLINK
// ============================================================

const unsigned long SELECTION_BLINK_INTERVAL = 500;

unsigned long lastSelectionBlink = 0;

bool selectionIndicatorVisible = true;


// ============================================================
// TEST RESULT STRUCTURE
// ============================================================

struct TestResult {

  // Cumulative voltages measured by the TLA2528
  float cumulative1;
  float cumulative2;
  float cumulative3;

  // Individual cell voltages
  float cell1;
  float cell2;
  float cell3;

  // Voltage measured from the separate PACK connector
  float pack;

  // Individual cell states
  bool cell1Pass;
  bool cell2Pass;
  bool cell3Pass;

  bool cell1Charge;
  bool cell2Charge;
  bool cell3Charge;

  // Overall cell result
  bool cellsPass;

  // Indicates whether any cell needs charging
  bool cellsNeedCharge;

  // Indicates whether any cell is over-voltage
  bool cellsOverVoltage;

  // PACK result
  bool packPass;

  // Final normal PASS result
  bool finalPass;
};


// ============================================================
// LCD LOW-LEVEL FUNCTIONS
// ============================================================

void lcdWrite4Bits(uint8_t data) {

  Wire.beginTransmission(LCD_ADDRESS);
  Wire.write(data | 0x08);
  Wire.endTransmission();

  delayMicroseconds(1);

  Wire.beginTransmission(LCD_ADDRESS);
  Wire.write(data | 0x0C);
  Wire.endTransmission();

  delayMicroseconds(1);

  Wire.beginTransmission(LCD_ADDRESS);
  Wire.write(data | 0x08);
  Wire.endTransmission();
}


void lcdSend(uint8_t value, uint8_t mode) {

  uint8_t high =
    value & 0xF0;

  uint8_t low =
    (value << 4) & 0xF0;

  lcdWrite4Bits(
    high | mode
  );

  lcdWrite4Bits(
    low | mode
  );
}


void lcdCommand(uint8_t command) {

  lcdSend(
    command,
    0x00
  );

  delayMicroseconds(50);
}


void lcdData(uint8_t data) {

  lcdSend(
    data,
    0x01
  );

  delayMicroseconds(50);
}


void lcdPrint(const char *text) {

  while (*text) {

    lcdData(*text);

    text++;
  }
}


void lcdSetCursor(
  uint8_t col,
  uint8_t row
) {

  uint8_t address;

  if (row == 0) {

    address = 0x00 + col;

  } else {

    address = 0x40 + col;
  }

  lcdCommand(
    0x80 | address
  );
}


void lcdClear() {

  lcdCommand(0x01);

  delay(2);
}


void lcdInit() {

  delay(50);

  lcdWrite4Bits(0x30);

  delay(5);

  lcdWrite4Bits(0x30);

  delayMicroseconds(150);

  lcdWrite4Bits(0x30);

  lcdWrite4Bits(0x20);

  lcdCommand(0x28);
  lcdCommand(0x08);
  lcdCommand(0x01);
  lcdCommand(0x06);
  lcdCommand(0x0C);
}


// ============================================================
// BUZZER FUNCTIONS
// ============================================================

// Generate one beep at 4 kHz.
void beepOnce(
  int duration = 200
) {

  tone(
    BUZZER_PIN,
    4000
  );

  delay(duration);

  noTone(
    BUZZER_PIN
  );
}


// PASS = exactly one beep.
void beepPass() {

  beepOnce(200);
}


// CHG / FAIL = exactly three beeps.
void beepFail() {

  beepOnce(200);

  delay(150);

  beepOnce(200);

  delay(150);

  beepOnce(200);
}


// ============================================================
// LED FUNCTIONS
// ============================================================

// Turn LED OFF.
void ledOff() {

  digitalWrite(
    LED_PIN,
    LOW
  );
}


// PASS state: LED stays continuously ON.
void ledPass() {

  digitalWrite(
    LED_PIN,
    HIGH
  );
}


// Start continuous blinking.
void startFailBlink() {

  failLedState = false;

  lastFailBlink = millis();

  digitalWrite(
    LED_PIN,
    LOW
  );
}


// Update blinking.
void updateFailBlink() {

  if (
    millis() -
    lastFailBlink >=
    FAIL_BLINK_INTERVAL
  ) {

    lastFailBlink =
      millis();

    failLedState =
      !failLedState;

    digitalWrite(
      LED_PIN,
      failLedState
        ? HIGH
        : LOW
    );
  }
}


// ============================================================
// BUTTON EVENT FUNCTION
// ============================================================
//
// Returns:
//
//   0 = no button event
//   1 = short press
//   2 = long press
//
// A long press is detected after 1.5 seconds even if the
// button is still being held.
//

int readButtonEvent() {

  bool state =
    digitalRead(
      BUTTON_PIN
    );

  int event = 0;


  // ----------------------------------------------------------
  // Button has just been pressed
  // ----------------------------------------------------------

  if (
    buttonPrevious == HIGH &&
    state == LOW
  ) {

    buttonPressed = true;

    longPressTriggered = false;

    buttonPressStart =
      millis();
  }


  // ----------------------------------------------------------
  // Detect long press
  // ----------------------------------------------------------

  if (
    buttonPressed &&
    state == LOW &&
    !longPressTriggered
  ) {

    if (
      millis() -
      buttonPressStart >=
      LONG_PRESS_TIME
    ) {

      longPressTriggered = true;

      event = 2;
    }
  }


  // ----------------------------------------------------------
  // Button has been released
  // ----------------------------------------------------------

  if (
    buttonPrevious == LOW &&
    state == HIGH
  ) {

    // If long press was not already triggered,
    // this is a short press.
    if (
      buttonPressed &&
      !longPressTriggered
    ) {

      event = 1;
    }

    buttonPressed = false;
  }


  buttonPrevious = state;

  return event;
}


// ============================================================
// RESET BUTTON STATE
// ============================================================

void resetButtonState() {

  buttonPrevious =
    digitalRead(
      BUTTON_PIN
    );

  buttonPressed = false;

  longPressTriggered = false;

  buttonPressStart = 0;
}


// ============================================================
// READ TLA2528 CHANNEL VOLTAGE
// ============================================================

float readChannelVoltage(
  uint8_t channel,
  float divider,
  float calibration
) {

  uint16_t raw16 =
    adc.read_one_channel_data(
      channel
    );


  // 0xFFFF indicates an I2C/read error in our library.
  if (
    raw16 == 0xFFFF
  ) {

    return -999.0;
  }


  // Convert 16-bit returned value to 12-bit ADC value.
  uint16_t raw12 =
    raw16 >> 4;


  // TLA2528 reference/input voltage is 3.3 V.
  float adcVoltage =
    (
      raw12 * 3.3
    ) / 4095.0;


  // Convert ADC voltage to actual battery voltage.
  float actualVoltage =
    adcVoltage *
    divider *
    calibration;


  return actualVoltage;
}


// ============================================================
// ROUND VOLTAGE TO TWO DECIMAL PLACES
// ============================================================
//
// This is important for the CHG decision.
//
// Example:
//
// Actual = 3.704 V
// Display = 3.70 V
//
// The displayed 3.70 V must be considered CHG.
//

float voltageForDisplay(
  float voltage
) {

  return round(
    voltage * 100.0
  ) / 100.0;
}


// ============================================================
// BATTERY PRESENT DETECTION
// ============================================================

bool batteryPresent() {

  float voltage =
    readChannelVoltage(
      0,
      DIVIDER_CH0,
      CAL_CH0
    );


  if (
    voltage < 1.0
  ) {

    return false;
  }


  return true;
}


// ============================================================
// CELL TEST
// ============================================================
//
// CH0 = Cell 1 cumulative voltage
//
// CH1 = Cell 1 + Cell 2 cumulative voltage
//
// CH2 = Cell 1 + Cell 2 + Cell 3 cumulative voltage
//
// Individual cells:
//
//   Cell 1 = CH0
//   Cell 2 = CH1 - CH0
//   Cell 3 = CH2 - CH1
//
// Cell status:
//
//   Displayed <= 3.70 V -> CHG
//
//   Actual > 4.20 V -> FAIL
//
//   Otherwise -> PASS
//
// ============================================================

void performCellTest(
  TestResult &result
) {

  // ----------------------------------------------------------
  // Read CH0
  // ----------------------------------------------------------

  result.cumulative1 =
    readChannelVoltage(
      0,
      DIVIDER_CH0,
      CAL_CH0
    );


  result.cumulative2 = 0;

  result.cumulative3 = 0;


  // Cell 1 is directly CH0.
  result.cell1 =
    result.cumulative1;

  result.cell2 = 0;

  result.cell3 = 0;


  // ----------------------------------------------------------
  // Read CH1 for 2S and 3S
  // ----------------------------------------------------------

  if (
    selectedBattery >=
    BATTERY_2S
  ) {

    result.cumulative2 =
      readChannelVoltage(
        1,
        DIVIDER_CH1,
        CAL_CH1
      );


    // Cell 2 is CH1 minus CH0.
    result.cell2 =
      result.cumulative2 -
      result.cumulative1;
  }


  // ----------------------------------------------------------
  // Read CH2 for 3S
  // ----------------------------------------------------------

  if (
    selectedBattery >=
    BATTERY_3S
  ) {

    result.cumulative3 =
      readChannelVoltage(
        2,
        DIVIDER_CH2,
        CAL_CH2
      );


    // Cell 3 is CH2 minus CH1.
    result.cell3 =
      result.cumulative3 -
      result.cumulative2;
  }


  // ----------------------------------------------------------
  // Initialize all states
  // ----------------------------------------------------------

  result.cell1Pass = true;
  result.cell2Pass = true;
  result.cell3Pass = true;

  result.cell1Charge = false;
  result.cell2Charge = false;
  result.cell3Charge = false;

  result.cellsOverVoltage = false;


  // ----------------------------------------------------------
  // CELL 1
  // ----------------------------------------------------------

  float cell1Display =
    voltageForDisplay(
      result.cell1
    );


  // 3.70 V displayed means CHG.
  if (
    cell1Display <=
    CELL_CHARGE_MIN_V
  ) {

    result.cell1Charge = true;

    result.cell1Pass = false;
  }


  // More than 4.20 V is FAIL.
  else if (
    result.cell1 >
    CELL_MAX_V
  ) {

    result.cell1Pass = false;

    result.cellsOverVoltage = true;
  }


  // ----------------------------------------------------------
  // CELL 2
  // ----------------------------------------------------------

  if (
    selectedBattery >=
    BATTERY_2S
  ) {

    float cell2Display =
      voltageForDisplay(
        result.cell2
      );


    if (
      cell2Display <=
      CELL_CHARGE_MIN_V
    ) {

      result.cell2Charge = true;

      result.cell2Pass = false;
    }

    else if (
      result.cell2 >
      CELL_MAX_V
    ) {

      result.cell2Pass = false;

      result.cellsOverVoltage = true;
    }
  }


  // ----------------------------------------------------------
  // CELL 3
  // ----------------------------------------------------------

  if (
    selectedBattery >=
    BATTERY_3S
  ) {

    float cell3Display =
      voltageForDisplay(
        result.cell3
      );


    if (
      cell3Display <=
      CELL_CHARGE_MIN_V
    ) {

      result.cell3Charge = true;

      result.cell3Pass = false;
    }

    else if (
      result.cell3 >
      CELL_MAX_V
    ) {

      result.cell3Pass = false;

      result.cellsOverVoltage = true;
    }
  }


  // ----------------------------------------------------------
  // Overall cell result
  // ----------------------------------------------------------

  result.cellsPass =
    result.cell1Pass &&
    result.cell2Pass &&
    result.cell3Pass;


  // ----------------------------------------------------------
  // Determine whether charging is required
  // ----------------------------------------------------------

  result.cellsNeedCharge =
    result.cell1Charge ||
    result.cell2Charge ||
    result.cell3Charge;
}


// ============================================================
// PACK TEST
// ============================================================
//
// CH3 is connected to the separate PACK connector.
//
// CH3 must match the total battery voltage:
//
//   1S -> CH0
//   2S -> CH1
//   3S -> CH2
//
// ============================================================

void performPackTest(
  TestResult &result
) {

  result.pack =
    readChannelVoltage(
      3,
      DIVIDER_CH3,
      CAL_CH3
    );


  float expectedPack = 0;


  // ----------------------------------------------------------
  // Determine expected PACK voltage
  // ----------------------------------------------------------

  if (
    selectedBattery ==
    BATTERY_1S
  ) {

    expectedPack =
      result.cumulative1;

  }

  else if (
    selectedBattery ==
    BATTERY_2S
  ) {

    expectedPack =
      result.cumulative2;

  }

  else {

    expectedPack =
      result.cumulative3;
  }


  // ----------------------------------------------------------
  // Invalid ADC reading
  // ----------------------------------------------------------

  if (
    result.pack < 0
  ) {

    result.packPass = false;

    return;
  }


  // ----------------------------------------------------------
  // Compare PACK with expected voltage
  // ----------------------------------------------------------

  float difference =
    result.pack -
    expectedPack;


  if (
    difference < 0
  ) {

    difference =
      -difference;
  }


  result.packPass =
    (
      difference <=
      PACK_MATCH_TOLERANCE
    );
}


// ============================================================
// CALCULATE FINAL RESULT
// ============================================================
//
// Normal PASS:
//
//   ALL active cells must be normal
//   AND PACK must pass.
//
// If any cell needs charging:
//
//   FINAL = CHG
//
// If any cell is above 4.20 V:
//
//   FINAL = FAIL
//
// If PACK fails:
//
//   FINAL = PACK FAIL
//
// ============================================================

void calculateFinalResult(
  TestResult &result
) {

  result.finalPass =
    result.cellsPass &&
    result.packPass;
}


// ============================================================
// DISPLAY TEST RESULT
// ============================================================
//
// 3S PASS:
//
//   3.77 3.79 3.75
//   11.30 PASS
//
// 3S CHG:
//
//   3.34 3.43 3.62
//   11.21 C1C2C3 CHG
//
// One cell CHG:
//
//   3.70 3.79 3.75
//   11.30 C1 CHG
//
// PACK FAIL:
//
//   3.77 3.79 3.75
//   11.30 PACK FAIL
//
// ============================================================

void displayResult(
  TestResult &result
) {

  char v1[8];
  char v2[8];
  char v3[8];
  char pack[8];


  lcdClear();


  // ----------------------------------------------------------
  // FIRST LINE - individual cell voltages
  // ----------------------------------------------------------

  lcdSetCursor(
    0,
    0
  );


  // Cell 1

  dtostrf(
    result.cell1,
    4,
    2,
    v1
  );

  lcdPrint(
    v1
  );


  // Cell 2

  if (
    selectedBattery >=
    BATTERY_2S
  ) {

    lcdPrint(
      "  "
    );


    dtostrf(
      result.cell2,
      4,
      2,
      v2
    );

    lcdPrint(
      v2
    );
  }


  // Cell 3

  if (
    selectedBattery >=
    BATTERY_3S
  ) {

    lcdPrint(
      "  "
    );


    dtostrf(
      result.cell3,
      4,
      2,
      v3
    );

    lcdPrint(
      v3
    );
  }


  // ----------------------------------------------------------
  // SECOND LINE
  // ----------------------------------------------------------

  lcdSetCursor(
    0,
    1
  );


  // PACK voltage WITHOUT "V".

  dtostrf(
    result.pack,
    4,
    2,
    pack
  );

  lcdPrint(
    pack
  );


  // ----------------------------------------------------------
  // PACK FAIL HAS PRIORITY
  // ----------------------------------------------------------

  if (
    !result.packPass
  ) {

    lcdPrint(
      " PACK FAIL"
    );

    return;
  }


  // ----------------------------------------------------------
  // OVER-VOLTAGE FAIL
  // ----------------------------------------------------------

  if (
    result.cellsOverVoltage
  ) {

    lcdPrint(
      " FAIL"
    );

    return;
  }


  // ----------------------------------------------------------
  // CHARGING REQUIRED
  // ----------------------------------------------------------

  if (
    result.cellsNeedCharge
  ) {

    lcdPrint(
      " "
    );


    // Cell 1
    if (
      result.cell1Charge
    ) {

      lcdPrint(
        "C1"
      );
    }


    // Cell 2
    if (
      selectedBattery >=
      BATTERY_2S &&
      result.cell2Charge
    ) {

      lcdPrint(
        "C2"
      );
    }


    // Cell 3
    if (
      selectedBattery >=
      BATTERY_3S &&
      result.cell3Charge
    ) {

      lcdPrint(
        "C3"
      );
    }


    lcdPrint(
      " CHG"
    );

    return;
  }


  // ----------------------------------------------------------
  // NORMAL PASS
  // ----------------------------------------------------------

  lcdPrint(
    " PASS"
  );
}


// ============================================================
// READY SCREEN
// ============================================================

void showReadyScreen() {

  lcdClear();


  lcdSetCursor(
    0,
    0
  );


  if (
    selectedBattery ==
    BATTERY_1S
  ) {

    lcdPrint(
      "1S LiPo Tester"
    );

  }

  else if (
    selectedBattery ==
    BATTERY_2S
  ) {

    lcdPrint(
      "2S LiPo Tester"
    );

  }

  else {

    lcdPrint(
      "3S LiPo Tester"
    );
  }


  lcdSetCursor(
    0,
    1
  );

  lcdPrint(
    "Press to test >>"
  );
}


// ============================================================
// BATTERY SELECTION SCREEN
// ============================================================

void showSelectionScreen() {

  lcdClear();


  lcdSetCursor(
    0,
    0
  );

  lcdPrint(
    "SELECT BATTERY"
  );


  lcdSetCursor(
    0,
    1
  );


  // Only the ">" indicator blinks.
  if (
    selectionIndicatorVisible
  ) {

    lcdPrint(
      "> "
    );

  } else {

    lcdPrint(
      "  "
    );
  }


  // The selected battery number is always displayed.

  if (
    selectedBattery ==
    BATTERY_1S
  ) {

    lcdPrint(
      "1S"
    );

  }

  else if (
    selectedBattery ==
    BATTERY_2S
  ) {

    lcdPrint(
      "2S"
    );

  }

  else {

    lcdPrint(
      "3S"
    );
  }
}


// ============================================================
// CHANGE BATTERY SELECTION
// ============================================================
//
// Short press cycles:
//
//   1S -> 2S -> 3S -> 1S
//

void nextBattery() {

  selectedBattery++;


  if (
    selectedBattery > 3
  ) {

    selectedBattery = 1;
  }


  Serial.print(
    "Selected battery: "
  );

  Serial.print(
    selectedBattery
  );

  Serial.println(
    "S"
  );


  // Restart the selection blink.

  selectionIndicatorVisible = true;

  lastSelectionBlink =
    millis();


  showSelectionScreen();
}


// ============================================================
// RUN COMPLETE BATTERY TEST
// ============================================================

void runTest() {

  TestResult result;


  Serial.println();

  Serial.println(
    "================================"
  );

  Serial.print(
    "STARTING "
  );

  Serial.print(
    selectedBattery
  );

  Serial.println(
    "S TEST"
  );

  Serial.println(
    "================================"
  );


  // ----------------------------------------------------------
  // TEST CELLS
  // ----------------------------------------------------------

  lcdClear();

  lcdSetCursor(
    0,
    0
  );

  lcdPrint(
    "Testing..."
  );


  delay(500);


  performCellTest(
    result
  );


  // ----------------------------------------------------------
  // TEST PACK
  // ----------------------------------------------------------

  performPackTest(
    result
  );


  // ----------------------------------------------------------
  // CALCULATE FINAL RESULT
  // ----------------------------------------------------------

  calculateFinalResult(
    result
  );


  // ----------------------------------------------------------
  // SERIAL OUTPUT
  // ----------------------------------------------------------

  Serial.println();

  Serial.println(
    "CELL RESULTS"
  );


  Serial.print(
    "S1 = "
  );

  Serial.print(
    result.cell1,
    3
  );

  Serial.print(
    " V  "
  );


  if (
    result.cell1Charge
  ) {

    Serial.println(
      "CHG"
    );

  }

  else {

    Serial.println(
      result.cell1Pass
        ? "PASS"
        : "FAIL"
    );
  }


  if (
    selectedBattery >=
    BATTERY_2S
  ) {

    Serial.print(
      "S2 = "
    );

    Serial.print(
      result.cell2,
      3
    );

    Serial.print(
      " V  "
    );


    if (
      result.cell2Charge
    ) {

      Serial.println(
        "CHG"
      );

    }

    else {

      Serial.println(
        result.cell2Pass
          ? "PASS"
          : "FAIL"
      );
    }
  }


  if (
    selectedBattery >=
    BATTERY_3S
  ) {

    Serial.print(
      "S3 = "
    );

    Serial.print(
      result.cell3,
      3
    );

    Serial.print(
      " V  "
    );


    if (
      result.cell3Charge
    ) {

      Serial.println(
        "CHG"
      );

    }

    else {

      Serial.println(
        result.cell3Pass
          ? "PASS"
          : "FAIL"
      );
    }
  }


  Serial.print(
    "PACK = "
  );

  Serial.print(
    result.pack,
    3
  );

  Serial.print(
    " V  "
  );

  Serial.println(
    result.packPass
      ? "PASS"
      : "FAIL"
  );


  // ----------------------------------------------------------
  // FINAL SERIAL RESULT
  // ----------------------------------------------------------

  Serial.print(
    "FINAL = "
  );


  if (
    !result.packPass
  ) {

    Serial.println(
      "PACK FAIL"
    );

  }

  else if (
    result.cellsOverVoltage
  ) {

    Serial.println(
      "FAIL"
    );

  }

  else if (
    result.cellsNeedCharge
  ) {

    Serial.println(
      "CHG"
    );

  }

  else {

    Serial.println(
      "PASS"
    );
  }


  // ----------------------------------------------------------
  // DISPLAY FINAL RESULT
  // ----------------------------------------------------------

  displayResult(
    result
  );


  // ----------------------------------------------------------
  // PASS
  // ----------------------------------------------------------

  if (
    result.finalPass
  ) {

    // LED stays ON continuously.
    ledPass();

    // One beep only.
    beepPass();


    Serial.println(
      "PASS: LED ON + 1 BEEP"
    );
  }


  // ----------------------------------------------------------
  // CHG
  // ----------------------------------------------------------

  else if (
    result.packPass &&
    result.cellsNeedCharge &&
    !result.cellsOverVoltage
  ) {

    // LED blinks continuously.
    startFailBlink();

    // Three beeps.
    beepFail();


    Serial.println(
      "CHG: LED BLINKING + 3 BEEPS"
    );
  }


  // ----------------------------------------------------------
  // FAIL
  // ----------------------------------------------------------

  else {

    // LED blinks continuously.
    startFailBlink();

    // Three beeps.
    beepFail();


    Serial.println(
      "FAIL: LED BLINKING + 3 BEEPS"
    );
  }


  // ----------------------------------------------------------
  // KEEP RESULT ON SCREEN
  // ----------------------------------------------------------

  resetButtonState();


  while (true) {

    // --------------------------------------------------------
    // AUTO DETECT BATTERY REMOVAL
    // --------------------------------------------------------

    if (
      !batteryPresent()
    ) {

      ledOff();

      noTone(
        BUZZER_PIN
      );


      Serial.println(
        "Battery removed."
      );


      // Return to READY screen.
      showReadyScreen();


      resetButtonState();

      return;
    }


    // --------------------------------------------------------
    // CONTINUE BLINKING FOR CHG / FAIL
    // --------------------------------------------------------

    if (
      !result.finalPass
    ) {

      updateFailBlink();
    }


    // --------------------------------------------------------
    // BUTTON
    // --------------------------------------------------------

    int event =
      readButtonEvent();


    // Long press -> return to battery selection.

    if (
      event == 2
    ) {

      ledOff();

      noTone(
        BUZZER_PIN
      );


      showSelectionScreen();


      selectionIndicatorVisible = true;

      lastSelectionBlink =
        millis();


      resetButtonState();

      return;
    }


    // Short press intentionally does nothing.

    delay(50);
  }
}


// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);


  // ----------------------------------------------------------
  // Configure hardware pins
  // ----------------------------------------------------------

  pinMode(
    BUTTON_PIN,
    INPUT_PULLUP
  );


  pinMode(
    LED_PIN,
    OUTPUT
  );


  pinMode(
    BUZZER_PIN,
    OUTPUT
  );


  ledOff();

  noTone(
    BUZZER_PIN
  );


  // ----------------------------------------------------------
  // Start I2C
  // ----------------------------------------------------------

  Wire.begin();

  delay(300);


  // ----------------------------------------------------------
  // Initialize LCD
  // ----------------------------------------------------------

  lcdInit();

  lcdClear();


  lcdSetCursor(
    0,
    0
  );

  lcdPrint(
    "LiPo Battery"
  );


  lcdSetCursor(
    0,
    1
  );

  lcdPrint(
    "Tester"
  );


  delay(1500);


  // ----------------------------------------------------------
  // Configure TLA2528 analog inputs
  //
  // AIN0 -> CH0
  // AIN1 -> CH1
  // AIN2 -> CH2
  // AIN3 -> CH3
  // ----------------------------------------------------------

  adc.write_to_register(

    PIN_CFG_ADDRESS,

    PIN_CFG_AIN0 |
    PIN_CFG_AIN1 |
    PIN_CFG_AIN2 |
    PIN_CFG_AIN3
  );


  delay(100);


  // ----------------------------------------------------------
  // Serial information
  // ----------------------------------------------------------

  Serial.println();

  Serial.println(
    "================================"
  );

  Serial.println(
    " LiPo Battery Tester"
  );

  Serial.println(
    "================================"
  );

  Serial.println(
    "System ready."
  );

  Serial.println(
    "Supported batteries: 1S / 2S / 3S"
  );


  // ----------------------------------------------------------
  // Start at battery selection screen
  // ----------------------------------------------------------

  selectionIndicatorVisible = true;

  lastSelectionBlink =
    millis();


  showSelectionScreen();

  resetButtonState();
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop() {

  int event =
    readButtonEvent();


  // ==========================================================
  // BATTERY SELECTION MODE
  // ==========================================================

  static bool inSelection = true;


  if (
    inSelection
  ) {

    // --------------------------------------------------------
    // Blink only the ">" indicator.
    // --------------------------------------------------------

    if (
      millis() -
      lastSelectionBlink >=
      SELECTION_BLINK_INTERVAL
    ) {

      lastSelectionBlink =
        millis();


      selectionIndicatorVisible =
        !selectionIndicatorVisible;


      showSelectionScreen();
    }


    // --------------------------------------------------------
    // SHORT PRESS = NEXT BATTERY
    // --------------------------------------------------------

    if (
      event == 1
    ) {

      nextBattery();

      delay(200);
    }


    // --------------------------------------------------------
    // LONG PRESS = CONFIRM SELECTION
    // --------------------------------------------------------

    if (
      event == 2
    ) {

      selectionIndicatorVisible = true;


      showReadyScreen();


      inSelection = false;


      resetButtonState();


      delay(200);
    }


    delay(10);

    return;
  }


  // ==========================================================
  // READY MODE
  // ==========================================================

  if (
    event == 1
  ) {

    runTest();


    resetButtonState();


    // runTest() returns after battery removal
    // or after a long press.
    inSelection = false;


    return;
  }


  // ----------------------------------------------------------
  // LONG PRESS = RETURN TO SELECTION
  // ----------------------------------------------------------

  if (
    event == 2
  ) {

    selectionIndicatorVisible = true;

    lastSelectionBlink =
      millis();


    showSelectionScreen();


    inSelection = true;


    resetButtonState();


    return;
  }


  delay(10);
}