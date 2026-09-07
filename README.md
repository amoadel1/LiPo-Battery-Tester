# LiPo Battery Tester

![Platform](https://img.shields.io/badge/Platform-Maker%20UNO-blue)
![ADC](https://img.shields.io/badge/ADC-TLA2528-green)
![Battery](https://img.shields.io/badge/Battery-1S%20%7C%202S%20%7C%203S-orange)
![Display](https://img.shields.io/badge/Display-16x2%20I2C-yellow)
![Status](https://img.shields.io/badge/Status-Completed-brightgreen)

A standalone LiPo Battery Tester designed to measure and evaluate 1S, 2S, and 3S LiPo batteries.

The tester uses a Cytron Maker UNO as the main controller and a TLA2528 external ADC for accurate battery voltage measurement. The system measures individual cell voltages, total battery voltage, and the PACK voltage, then provides a PASS or FAIL result using an LCD, LED indicator, and buzzer.

---

# Project Overview

The LiPo Battery Tester was developed as an independent hardware and software project.

The system is designed to test:

- 1S LiPo batteries
- 2S LiPo batteries
- 3S LiPo batteries

The tester measures the cumulative voltage of the battery cells through an external TLA2528 ADC.

For multi-cell batteries, the individual cell voltages are calculated from the cumulative measurements.

For example, for a 3S battery:

```text
Cell 1 = CH0

Cell 2 = CH1 - CH0

Cell 3 = CH2 - CH1
```

The PACK voltage is measured separately through a dedicated PACK connector.

The final battery result is:

```text
PASS
```

only when all individual cells and the PACK voltage pass the programmed test conditions.

If any cell or the PACK voltage fails, the final result is:

```text
FAIL
```

---

# Features

- 1S LiPo battery testing
- 2S LiPo battery testing
- 3S LiPo battery testing
- Individual cell voltage measurement
- Total battery voltage measurement
- Separate PACK voltage measurement
- External TLA2528 12-bit ADC
- I2C communication
- 16x2 I2C LCD
- PASS indication
- FAIL indication
- LED status indication
- Buzzer indication
- Battery removal detection
- Battery selection menu
- Short press control
- Long press control
- Dedicated ON/OFF power switch
- Voltage divider protection and scaling
- Regulated power supply
- Compact assembled enclosure
- Dedicated battery connector holder
- Dedicated PACK connector holder
- Dedicated LCD holder

---

# Hardware Components

The main hardware components used in this project include:

- Cytron Maker UNO
- TLA2528 external ADC
- 16x2 I2C LCD
- Combined LED and push-button module
- Buzzer
- DC-DC voltage regulator
- ON/OFF power switch
- 1S LiPo battery connector
- 2S LiPo battery connector
- 3S LiPo battery connector
- Separate 2-wire PACK connector
- Voltage divider resistor networks
- Battery connector holder
- LCD enclosure
- Connecting wires
- Terminal blocks and connectors
- LiPo batteries for testing

---

# Complete Hardware Assembly

The complete tester was assembled on a mounting board.

The main assembly includes the Maker UNO, TLA2528 ADC, voltage regulator, LCD enclosure, power switch, battery connectors, PACK connector, LED/button module, and wiring.

![Complete Hardware Assembly](./images/complete_hardware_assembly.jpg)

**Figure 1.** Complete hardware assembly of the LiPo Battery Tester.

---

# Maker UNO

The Cytron Maker UNO is used as the main controller of the tester.

The Maker UNO handles:

- User button input
- LED control
- Buzzer control
- LCD communication
- TLA2528 communication
- Battery selection
- Voltage calculations
- PASS/FAIL evaluation
- Battery removal detection

The Maker UNO is powered from a regulated:

```text
5V
```

---

# Power Supply

The tester uses an external DC power input.

A DC-DC voltage regulator is used to provide the required supply voltage.

The regulator output was adjusted and verified at:

```text
5V
```

The regulated 5V supply is used to power the Maker UNO and the LCD.

The TLA2528 ADC is powered separately from:

```text
3.3V
```

A dedicated ON/OFF power switch is included in the main power circuit.

---

# TLA2528 ADC

The TLA2528 is used as the external ADC for measuring the battery voltages.

The ADC communicates with the Maker UNO using the I2C interface.

The TLA2528 is powered from:

```text
3.3V
```

The ADC is configured to measure the voltage divider outputs connected to its analog input channels.

The TLA2528 I2C address used in this project is:

```text
0x10
```

The TLA2528 provides 12-bit ADC measurements for the battery voltage sensing channels.

---

# LCD Display

A 16x2 I2C LCD is used to display:

- Battery selection
- Tester status
- Individual cell voltages
- PACK voltage
- PASS result
- FAIL result

The LCD I2C address is:

```text
0x27
```

The LCD is mounted inside a dedicated enclosure.

![LCD Enclosure](./images/lcd_enclosure.jpg)

**Figure 2.** LCD mounted in the dedicated tester enclosure.

---

# LED and Push Button

A combined LED and push-button module is used as the user interface.

The module has three wires:

- LED signal
- Button signal
- Common GND

The current connections are:

| Component | Maker UNO Pin |
|---|---|
| LED | D4 |
| Button | D7 |
| Common | GND |

The LED provides the visual PASS/FAIL indication.

The button is used to navigate the battery selection and start the test.

---

# Buzzer

The Maker UNO onboard buzzer is used for audible test indications.

The buzzer is connected to:

```text
D8
```

The buzzer provides:

- One beep for PASS
- Three beeps for FAIL

The FAIL indication produces three buzzer beeps once when the test result is generated.

---

# Battery Connectors

The tester supports three battery configurations:

```text
1S
2S
3S
```

The battery connectors were combined into a common four-wire arrangement for the cumulative cell measurements.

The actual wire arrangement used in the assembled tester is:

| Wire | Function |
|---|---|
| Black | GND / B- |
| Red | S1 cumulative |
| Blue | S2 cumulative |
| Green | S3 cumulative / PACK+ |

The cumulative measurements are connected to the TLA2528 ADC channels.

---

# ADC Channel Assignment

The TLA2528 channels are assigned as follows:

| ADC Channel | Measurement |
|---|---|
| CH0 | S1 cumulative voltage |
| CH1 | S1 + S2 cumulative voltage |
| CH2 | S1 + S2 + S3 cumulative voltage |
| CH3 | Separate PACK connector |

For a 3S battery:

```text
CH0 = Cell 1

CH1 = Cell 1 + Cell 2

CH2 = Cell 1 + Cell 2 + Cell 3

CH3 = PACK voltage
```

---

# Individual Cell Voltage Calculation

The individual cell voltages are calculated from the cumulative measurements.

## Cell 1

```text
Cell 1 = CH0
```

## Cell 2

```text
Cell 2 = CH1 - CH0
```

## Cell 3

```text
Cell 3 = CH2 - CH1
```

The total battery voltage is obtained from the cumulative third-cell measurement for a 3S battery.

The PACK voltage is measured separately through CH3.

---

# Voltage Divider Configuration

Voltage dividers are used to scale the battery voltages before they are applied to the TLA2528 ADC inputs.

The voltage divider configuration used in the project is:

| Channel | R1 | R2 | Divider Multiplier |
|---|---:|---:|---:|
| CH0 | 10kΩ | 10kΩ | 2.000000 |
| CH1 | 20kΩ | 10kΩ | 3.000000 |
| CH2 | 15kΩ | 3.9kΩ | 4.846154 |
| CH3 | 15kΩ | 3.9kΩ | 4.846154 |

The CH2 and CH3 divider multiplier is based on:

```text
(15kΩ + 3.9kΩ) / 3.9kΩ

= 4.846154
```

---

# Calibration

Calibration constants are included in the software to compensate for small measurement differences.

The calibration values used are:

| Channel | Calibration |
|---|---:|
| CH0 | 0.98875 |
| CH1 | 1.00316 |
| CH2 | 1.00988 |
| CH3 | 1.00988 |

These values are applied during the voltage calculation.

---

# User Interface

The tester uses a simple menu system.

The user first selects the battery type.

The selection screen displays:

```text
SELECT BATTERY
> 2S
```

The available selections are:

```text
1S
2S
3S
```

The selection indicator blinks while the battery type is being selected.

The selected battery type remains visible while the selection indicator changes.

---

# Battery Selection

The button is used to select the required battery type.

The selection sequence is:

```text
1S → 2S → 3S → 1S
```

A short press cycles through the available battery types.

A long press confirms the selected battery type.

After confirmation, the tester displays the ready screen.

---

# Ready Screen

After selecting the battery type, the tester displays:

```text
3S LiPo Tester
Press to test >>
```

The same format is used for 1S and 2S:

```text
1S LiPo Tester
Press to test >>
```

or:

```text
2S LiPo Tester
Press to test >>
```

A short press starts the battery test.

A long press returns to the battery selection screen.

---

# Testing Process

After the user presses the button to start the test, the Maker UNO reads the required TLA2528 ADC channels.

The system then:

1. Reads the ADC channels.
2. Converts the ADC readings to voltage.
3. Applies the voltage divider ratios.
4. Applies the calibration factors.
5. Calculates individual cell voltages.
6. Calculates the total battery voltage.
7. Reads the separate PACK voltage.
8. Checks every individual cell.
9. Checks the PACK voltage.
10. Generates the final PASS or FAIL result.
11. Controls the LED and buzzer according to the result.

---

# 1S Battery Test

For a 1S battery, the tester measures the first cell.

The result screen displays the cell voltage and total voltage.

Example:

```text
3.76
3.76 V [PASS]
```

---

# 2S Battery Test

For a 2S battery, the tester calculates:

```text
Cell 1 = CH0

Cell 2 = CH1 - CH0
```

The display shows both cell voltages and the total battery voltage.

Example:

```text
3.56  3.76
7.45V [FAIL]
```

---

# 3S Battery Test

For a 3S battery, the tester calculates:

```text
Cell 1 = CH0

Cell 2 = CH1 - CH0

Cell 3 = CH2 - CH1
```

The display shows all three individual cell voltages and the total battery voltage.

Example:

```text
3.76  3.81  3.75
11.54V [PASS]
```

---

# PASS Condition

The battery receives a:

```text
PASS
```

result only when:

- All individual cells pass
- The PACK voltage passes

The LED remains ON for a PASS result.

The buzzer produces one beep.

Example:

```text
3.77  3.85  3.84
11.53V [PASS]
```

---

# FAIL Condition

The battery receives a:

```text
FAIL
```

result if:

- Any individual cell fails
- Or the PACK voltage fails

The LED blinks continuously for a FAIL result.

The buzzer sounds three times once when the FAIL result is generated.

Example:

```text
3.77  3.85  3.84
11.15V [FAIL]
```

---

# Battery Removal Detection

The tester includes automatic battery removal detection.

After displaying the test result, the result screen remains stable while the battery is connected.

When the battery is removed, the tester detects the removal and automatically returns to the ready screen.

This allows the tester to be used repeatedly without manually resetting the system.

---

# Long Press Function

The button supports long-press operation.

From the ready screen:

```text
Long Press → Return to Battery Selection
```

From the result screen:

```text
Long Press → Return to Battery Selection
```

A short press from the result screen does not change the result.

---

# Test Measurements

The tester was physically tested using 1S, 2S, and 3S LiPo batteries.

The measured values demonstrated that the system can successfully read individual cell voltages and total battery voltage.

---

# 2S Battery Testing

A 2S battery was tested and the tester produced results such as:

```text
3.88  3.94
7.90V [PASS]
```

Another 2S test produced:

```text
3.95  4.01
8.05V [PASS]
```

The measurements show that the tester can measure the two individual cells separately and calculate the total voltage.

![2S Battery Test](./images/2s_battery_test.jpg)

**Figure 3.** 2S LiPo battery test result.

---

# 3S Battery Testing

A 3S LiPo battery was tested using the completed tester.

One successful measurement produced approximately:

```text
Cell 1 = 3.84V
Cell 2 = 3.85V
Cell 3 = 3.77V
PACK   = 11.52V
```

The sum of the displayed cell voltages was:

```text
3.84 + 3.85 + 3.77 = 11.46V
```

The independently measured PACK voltage was approximately:

```text
11.52V
```

The difference was approximately:

```text
0.06V
```

The tester successfully generated a PASS result when all programmed conditions were satisfied.

![3S Battery Test](./images/3s_battery_test.jpg)

**Figure 4.** 3S LiPo battery test result.

---

# Additional 3S Test

Another 3S battery measurement produced approximately:

```text
Cell 1 = 3.78V
Cell 2 = 3.79V
Cell 3 = 3.75V
```

with a total voltage of approximately:

```text
11.30V
```

The tester successfully displayed the three individual cell measurements and the total battery voltage.

![3S Test Result](./images/3s_test_result.jpg)

**Figure 5.** Additional 3S battery test.

---

# PASS and FAIL Verification

The PASS and FAIL functions were also tested.

The tester successfully displayed PASS when the required measurements were within the programmed conditions.

The FAIL function was also verified by testing conditions where a cell or battery voltage did not satisfy the required limits.

The FAIL indication includes:

```text
LED blinking
```

and:

```text
Three buzzer beeps
```

The final result is based on both the cell measurements and the PACK measurement.

---

# Hardware Assembly Process

The complete physical assembly was carried out as part of the project development.

The main assembly work included:

- Soldering the button wires
- Soldering the voltage regulator wires
- Installing the dedicated ON/OFF power switch
- Connecting the power circuit
- Adjusting the regulator output to 5V
- Verifying the regulated 5V output
- Connecting the 5V supply to the Maker UNO
- Mounting the Maker UNO and shield
- Installing the TLA2528 ADC
- Installing the LCD
- Making a dedicated LCD holder
- Making a dedicated PACK connector holder
- Organizing the battery connector wiring
- Connecting the battery sensing wires
- Organizing the wiring using cable ties
- Completing the main physical assembly
- Testing the complete system with LiPo batteries

![Hardware Assembly](./images/hardware_assembly.jpg)

**Figure 6.** Hardware assembly and wiring of the tester.

---

# Final Physical Design

The final tester consists of the main electronics mounted on a wooden base together with the LCD enclosure, power switch, battery connectors, and control button.

The LCD is mounted at the front for easy viewing.

The large red button provides a simple user interface for selecting and testing batteries.

The battery connectors are positioned on the side of the tester for convenient battery connection.

![Final Tester](./images/final_tester.jpg)

**Figure 7.** Final assembled LiPo Battery Tester.

---

# System Block Diagram

The overall system can be represented as:

```text
                LiPo Battery
                     |
          +----------+----------+
          |                     |
          | Cell Taps           | PACK Connector
          |                     |
          v                     v
   Voltage Dividers          CH3 Divider
          |                     |
          +----------+----------+
                     |
                     v
                TLA2528 ADC
                  I2C
                     |
                     v
                Maker UNO
          +----------+----------+
          |          |          |
          v          v          v
         LCD        LED       Buzzer
          |
          v
      Test Result
      PASS / FAIL
```

---

# Measurement System

The measurement system uses the following structure:

```text
Battery
   |
   v
Voltage Divider
   |
   v
TLA2528 ADC
   |
   | I2C
   v
Maker UNO
   |
   +----> Voltage Calculation
   |
   +----> Cell Calculation
   |
   +----> PASS / FAIL Evaluation
   |
   +----> LCD
   |
   +----> LED
   |
   +----> Buzzer
```

---

# Software

The tester was programmed using the Arduino IDE.

The main program file is:

```text
LiPo_battery_tester.ino
```

The complete Arduino source code is available in the `code` folder.

**Complete Code:**

[LiPo_battery_tester.ino](./code/LiPo_battery_tester.ino)

---

# Software Structure

The software is responsible for:

- Initializing the Maker UNO
- Initializing I2C
- Initializing the TLA2528
- Initializing the LCD
- Configuring the LED
- Configuring the button
- Configuring the buzzer
- Displaying the battery selection menu
- Handling short presses
- Handling long presses
- Starting the battery test
- Reading ADC channels
- Converting ADC readings into voltages
- Applying divider ratios
- Applying calibration factors
- Calculating individual cells
- Reading PACK voltage
- Checking the battery condition
- Displaying PASS or FAIL
- Controlling the LED
- Controlling the buzzer
- Detecting battery removal
- Returning to the ready screen

---

# Important I2C Addresses

The I2C addresses used in this project are:

| Device | I2C Address |
|---|---|
| TLA2528 ADC | `0x10` |
| 16x2 LCD | `0x27` |

The TLA2528 address `0x10` belongs specifically to this LiPo Battery Tester project.

---

# Main Pin Configuration

The main control pins used by the project are:

| Function | Maker UNO Pin |
|---|---|
| LED | D4 |
| Button | D7 |
| Buzzer | D8 |
| I2C SDA | SDA / A4 |
| I2C SCL | SCL / A5 |

The TLA2528 communicates through the I2C interface.

---

# Battery Measurement Channels

| Channel | Function |
|---|---|
| CH0 | S1 cumulative voltage |
| CH1 | S1 + S2 cumulative voltage |
| CH2 | S1 + S2 + S3 cumulative voltage |
| CH3 | Separate PACK voltage |

---

# Power Configuration

The power configuration is:

```text
DC Input
   |
   v
ON/OFF Switch
   |
   v
DC-DC Regulator
   |
   +----> 5V ----> Maker UNO
   |
   +----> 5V ----> LCD

Separate 3.3V Supply
   |
   v
TLA2528 ADC
```

The TLA2528 must be powered from 3.3V in this project.

---

# Safety

LiPo batteries can deliver high currents and must be handled carefully.

Before connecting a battery:

- Verify the battery connector wiring.
- Verify the polarity.
- Verify the common GND connection.
- Check for accidental short circuits.
- Confirm that the voltage divider wiring is correct.
- Confirm that the TLA2528 supply is 3.3V.
- Do not connect a damaged battery.
- Do not continue testing if abnormal heating, smoke, sparks, or unusual behavior occurs.

The battery should only be connected after the tester wiring has been verified.

---

# Project Testing

The completed tester was tested with different battery configurations.

The testing confirmed:

- Correct battery selection
- Correct 1S operation
- Correct 2S operation
- Correct 3S operation
- Individual cell voltage calculation
- Total voltage measurement
- PACK voltage measurement
- LCD display operation
- LED operation
- Buzzer operation
- PASS indication
- FAIL indication
- Battery removal detection
- Long press navigation
- Stable result display

---

# Final Result

The LiPo Battery Tester was successfully assembled, programmed, and tested.

The final system can identify the selected battery configuration, measure the required cell voltages, measure the PACK voltage, calculate individual cell voltages, and provide a PASS or FAIL result.

The completed hardware includes the Maker UNO, TLA2528 ADC, LCD, LED/button module, buzzer, regulated power supply, battery connectors, PACK connector, and dedicated enclosure.

The project successfully demonstrates a complete standalone LiPo battery testing system.

---

---

# Demonstration

A demonstration video can be added here:

**YouTube Demo:**

[Add YouTube Demo Link Here](#)

---

# Author

**Adel Husham Mohamedain**

Electrical Engineering Student  
Universiti Malaysia Perlis (UniMAP)
