// *** Automatic Plant Irrigation System ***
// Code for Tech Day Project by Abdulrahman Yousef
// To be used with the Arduino UNO R3
// Created: 4/11/2025
// Last modified: 4/18/2025

#include <LiquidCrystal.h>

const int rsPin = 7;
const int enablePin = 6;
const int d4 = 10;
const int d5 = 11;
const int d6 = 12;
const int d7 = 13;

LiquidCrystal lcd(rsPin, enablePin, d4, d5, d6, d7);

const int lcdMainPower = 2;
const int lowWaterAlarmPin = 3;
const int queryButtonPower = 4;
const int soilMoisturePower = 5;
const int relaySignalPin = 8;
const int queryButtonRead = 9;

const int soilMoisturePin = A0;
const int waterSensorPin = A1;
const int waterSensorPower = A2;
const int lcdBacklightControl = A4;
const int lowWaterAlarmLed = A5;

// change this to control the interval
// soil moisture and water level data
// are reported.
const int reportCycleDurationMs = 5000;
//*********************************************

// The value below represents how many report -
// cycles would pass before the system checks
// for moisture and water levels and acts
// accordingly.
//
// multiply this by reportCycleDurationMs and
// you get the time before the system
// performs its main check.
const int reportCyclesBeforeMainCheck = 120;
//*********************************************

// Represents how many report cycles would pass
// before the power saver kicks in.
//
// Multiply this value by reportCycleDurationMs
// and you get the time before the power saver
// activates.
const int reportCyclesBeforePowerSaver = 10;
//*********************************************

unsigned int reportCycle = 0;
bool lcdIsOn = true;
bool lcdJustQueried = false;
bool firstStartup = true;

const int getMoistureLevel() {
    int tempMoistureLevel = 0;
    digitalWrite(soilMoisturePower, HIGH);
    delay(110);
    tempMoistureLevel = analogRead(soilMoisturePin);
    digitalWrite(soilMoisturePower, LOW);
    return tempMoistureLevel;
}
// 110 ms is just an arbitrary number
// of milliseconds. < 100 is usually fine.
const int getWaterLevel() {
    int tempWaterLevel = 0;
    digitalWrite(waterSensorPower, HIGH);
    delay(110);
    tempWaterLevel = analogRead(waterSensorPin);
    analogWrite(waterSensorPower, LOW);
    return tempWaterLevel;
}

void turnOffLcd() {
    if (!lcdIsOn) { return; }
    lcd.noDisplay();
    pinMode(rsPin, INPUT);
    pinMode(enablePin, INPUT);
    // probably not necessary
    pinMode(d4, INPUT);
    pinMode(d5, INPUT);
    pinMode(d6, INPUT);
    pinMode(d7, INPUT);
    //***********************
    delay(250);
    digitalWrite(lcdMainPower, LOW);
    digitalWrite(lcdBacklightControl, LOW);
    lcdIsOn = false;
}

void turnOnLcd() {
    /*
    if (!lcdIsOn && digitalRead(lcdMainPower) == HIGH) {
        tone(lowWaterAlarmPin, 500, 1000);
    }
    else if (lcdIsOn && digitalRead(lcdMainPower) == LOW) {
        tone(lowWaterAlarmPin, 500, 1000);
    }*/

    if (!lcdIsOn) {
        lcd.begin(16, 2);
        digitalWrite(lcdMainPower, HIGH);
        digitalWrite(lcdBacklightControl, HIGH);
        lcd.display();
        lcdIsOn = true;
        delay(100); // Give time for the LCD to start.
    }
}

void setup() {
    Serial.begin(9600);
    lcd.begin(16, 2); // 16 columns and 2 rows
    lcd.autoscroll();

    pinMode(soilMoisturePin, INPUT);
    pinMode(waterSensorPin, INPUT);
    pinMode(waterSensorPower, OUTPUT);

    pinMode(relaySignalPin, OUTPUT);
    pinMode(soilMoisturePower, OUTPUT);
    pinMode(lowWaterAlarmPin, OUTPUT);
    pinMode(lcdMainPower, OUTPUT);
    pinMode(lowWaterAlarmLed, OUTPUT);
    pinMode(lcdBacklightControl, OUTPUT);

    pinMode(queryButtonPower, OUTPUT);
    pinMode(queryButtonRead, INPUT_PULLUP);

    digitalWrite(lcdMainPower, HIGH);
    digitalWrite(lcdBacklightControl, HIGH);
    digitalWrite(queryButtonPower, LOW);
}

void loop() {
    /*
    * Soil moisture sensors work a bit differently,
    * as in they report increased moisture close
    * to zero. i.e (fully moist == 0) and -
    * (fully dry == 100 [or any max value])
    * this is why we subtract 100 which is the max
    * output value from the map() function set here.
    */

    // The fromMax value used here is based on a quick
    // calliberation for the sensor.
    int moistureLevel = 100 - map(getMoistureLevel(), 0, 980, 0, 100);
    // The fromMax value used here is based on a quick
    // calliberation for the sensor.
    int waterLevel = map(getWaterLevel(), 0, 580, 0, 100);

    /*
    * Turn off the LCD after 10 cycles
    * (50 seconds in our case; 5 seconds * 10)
    * to save power.
    */
    if (reportCycle > 0 && reportCycle % reportCyclesBeforePowerSaver == 0 && !lcdJustQueried) {
        turnOffLcd();
        lcdJustQueried = true;
    }
    else if (lcdJustQueried) {
        lcdJustQueried = false;
        /*
        * This below makes sure that when a
        * user queries data on the LCD,
        * The lcd should shut off again exactly 9
        * check cycles later. Not before. This is done by
        * subtracting a number from the reportCycle
        * and performing a modulo operation on it
        * where the result would equal 1.
        * Example: (25 - 4) % 10 == 1. This would
        * ensure that there is 9 check cycles
        * worth of time before the LCD turns off
        * to give users enough time to view
        * the readings on the LCD.
        */

        //! !HASN'T BEEN TESTED FOR ODD NUMBERS,
        //! PRIME NUMBERS, OR LARGER NUMBERS!
        for (int x = 1; x < reportCyclesBeforePowerSaver; ++x) {
            int checkCycleCopy = reportCycle;
            if ((checkCycleCopy - x) % reportCyclesBeforePowerSaver == 1) {
                reportCycle -= x;
                break;
            }
        }
    }

    // Give time for the LCD to start during
    // first boot. Might not be neccessary too
    if (firstStartup) {
        delay(1000);
        firstStartup = false;
    }
    // every 120 cycles, each being 5 seconds (10 mins)
    // in this case (120 * 5000ms == 600000ms == 10mins)
    if (reportCycle % reportCyclesBeforeMainCheck == 0 && moistureLevel <= 15) {
        Serial.print("Moisture level: ");
        Serial.println(moistureLevel);
        Serial.print("Water level: ");
        Serial.println(waterLevel);
        
        for (int cycle = 0; waterLevel <= 10; cycle++) {
            waterLevel = getWaterLevel();
            turnOnLcd();
            if (waterLevel > 10) {
                lcd.clear();
                lcd.home();
                lcd.print("Thank you! :)");
                break;
            }

            tone(lowWaterAlarmPin, 1000, 500);
            digitalWrite(lowWaterAlarmLed, HIGH);
            delay(890);
            digitalWrite(lowWaterAlarmLed, LOW);

            // interval pause for low water alarm.
            if (cycle > 0 && cycle % 10 == 0) {
                delay(2000);
            }

            // alternate between low water alarm messages
            if (cycle % 4 == 0) {
                lcd.clear();
                lcd.home();
                lcd.print("Please refill");
                lcd.setCursor(0, 1);
                lcd.print("water.");
            }
            else if (cycle % 2 == 0) {
                lcd.clear();
                lcd.home();
                lcd.print("Water too low!");
            }
            lcd.noCursor(); // to prevent blinking cursor
                            // each time LiquidCrystal::home()
                            // is called.
        }
        // Pump water for 1.25 seconds
        digitalWrite(relaySignalPin, HIGH);
        delay(1250);
        digitalWrite(relaySignalPin, LOW);
        delay(1250);
    }

    for (int ms = 1; ms <= reportCycleDurationMs; ++ms) {
        /*
        * The queryButtonRead works in reverse.
        * LOW == HIGH and vice-versa
        * This is because the pin's mode is set
        * to INPUT_PULLUP
        */
        if (!lcdIsOn && digitalRead(queryButtonRead) == LOW) {
            digitalWrite(lcdMainPower, HIGH);
            delay(250);
            lcd.flush();
            lcd.display();
            lcdIsOn = true;
            lcdJustQueried = true;
        }
        delay(1);
    }

    Serial.print("Cycle number: ");
    Serial.println(reportCycle);

    if (lcdIsOn) {
        lcd.flush(); // Might be unnecessary
        lcd.clear();
        lcd.home();
        lcd.print("Moisture: ");
        lcd.print(moistureLevel);
        lcd.print('\%');
        lcd.setCursor(0, 1);
        lcd.print("Water level: ");
        lcd.print(waterLevel);
        lcd.print('\%');
        lcd.home();
        lcd.noCursor();
    }

    ++reportCycle;
    if (reportCycle > reportCyclesBeforeMainCheck) {
        reportCycle = 1; // reset report cycles
    }
}