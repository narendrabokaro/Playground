/*************************************************************
  Filename - HomeAutomationProjectAV1
  Description - Room automation project which involves bathroom, fan and two LED bulb control
  SPECIAL INSTRUCTION - These changes required reconfiguring the GPIO pin.
  version - AV 1.0.1
  type - Arduino version
  Updates/ Fixes -
  > Introduced a new function - kitchen_control for managing the sarita requirement.
  > bring stability in other functions

  Debug instructions -
  1> Always check the active hours for bathroom lighting. Standard time - 6pm to 7am
  2> Check for bathroom light duration in minute. means how long the bulb will glow once turned ON
 *************************************************************/

// For RTC module
#include "RTClib.h"

// Basic configuration variables
// moved the variable init from setup to loop() section
bool isInitialSetupPerformed = false;

// Tell whether LED bulb is On/ Off
boolean isBathroomLedOn = false;
boolean isKitchenLedOn = false;

// contain status of motion sensor i.e. 1 if activity detected/ 0 silent
long motionSensorStatus;

// Bathroom sensor - Active hours
int activeHourStart = 18;   // 2.00 pm
int activeHourEnd = 7;      // 7.00 am

int bathroomLightOnDuration = 2;    // In minutes
int kitchenLightOnDuration = 1;    // In minutes
int iAmStillInToilet = 0;

// To maintain the active alarms
struct alarm {
    int alarmId;
    int alarmType;  // 1 - Daily, 2 - Weekly, 3 - Monthly
    // Flag indicates alarm status - Enabled/disabled
    int isAlarmSet;
    // Flag indicate whether alarm triggered or not
    int isAlarmTriggered;
    int duration;
    char whomToActivate[25];

    int endTimeHour;
    int endTimeMinute;
    int endTimeSecond;
};

// Created this struct to maintain more than one alarms i.e. one for bathroom and another for bedroom bulb
// 0 - bathroom timer
// 1 - bedroom timer
struct alarm activeAlarm[2];

/* GPIO Pin configuration details */
/* ------------------------------ */
// Sensors/ Relay connection details with GPIOs pins
// D1 and D2 allotted to RTC module DS1307

#define windowBulbRelay 2
#define bathroomMotionSensor 3
#define bathroomBulbRelay 4
// Variable used for Outside bathroom - Window bulb and switch
#define windowBulbSwitch 5
#define iAmStillInToiletSwitch 6

// RTC Module
// On arduino > connected to I2C pins (above ARFT)
RTC_DS1307 rtc;
DateTime currentTime;

// Window Led Relay State
bool windowLedRelayState = LOW; 

// Switch State - ON (HIGH)/ OFF (LOW)
bool windowBulbSwitchState = LOW;

/*
* This function write data into two places - NodeMCU file system (for offline support) and for google sheet API
*/
void actionMessageLogger(String message) {
    Serial.println(message);
}

// turnBulb("ON", "OutBulb")
void turnBulb(String action, String bulbLocation) {
    if (bulbLocation == "OutBulb") {
        // Turn ON the bulb by making relay LOW else
        digitalWrite(windowBulbRelay, action == "ON" ? LOW : HIGH);
        actionMessageLogger(action == "ON" ? "windowBulbRelay :: Turn ON the window bulb" : "windowBulbRelay :: Turn OFF the window bulb");
    }

    if (bulbLocation == "bathroomBulb") {
        // Turn ON the bulb by making relay LOW
        // Turn OFF the bulb by making relay HIGH
        digitalWrite(bathroomBulbRelay, action == "ON" ? LOW : HIGH);
        actionMessageLogger(action == "ON" ? "bathroomBulbRelay :: Turn ON the bathroom bulb" : "bathroomBulbRelay :: Turn OFF the bathroom bulb");
    }

    if (bulbLocation == "kitchenBulb") {
        // Turn ON the bulb by making relay LOW
        // Turn OFF the bulb by making relay HIGH
        digitalWrite(windowBulbRelay, action == "ON" ? LOW : HIGH);
        actionMessageLogger(action == "ON" ? "KitchenBulb :: Turn ON the kitchen bulb" : "KitchenBulb :: Turn OFF the kitchen bulb");
    }
}

// For RTC module setup
void rtcSetup() {
    Serial.println("rtcSetup :: Health status check");
 
    if (!rtc.begin()) {
        Serial.println("rtcSetup :: Couldn't find RTC");
        Serial.flush();
        while (1) delay(10);
    }
    
    if (! rtc.isrunning()) {
        Serial.println("rtcSetup :: RTC is NOT running, let's set the time!");
        // When time needs to be set on a new device, or after a power loss, the
        // following line sets the RTC to the date & time this sketch was compiled
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        // This line sets the RTC with an explicit date & time, for example to set
        // January 21, 2014 at 3am you would call:
        // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    }

    Serial.println("rtcSetup :: RTC is running fine and Current time >");
    currentTime = rtc.now();
    Serial.print(currentTime.hour());
    Serial.print(":");
    Serial.print(currentTime.minute());
    Serial.print(":");
    Serial.print(currentTime.second());
}

void unsetAlarm(int alarmId) {
    Serial.println("Removed the alarm");
    activeAlarm[alarmId].isAlarmSet = 0;
    // Set this flag so that next time, it can be set again
    activeAlarm[alarmId].isAlarmTriggered = 1;
}

// duration = for how long you want to set alarm
// alarmType = 1 for hour | 2 for Minute
void setAlarm(int alarmId, int duration, int alarmType) {
    int temp = 0;
    // Bathroom Timer duration
    activeAlarm[alarmId].alarmId = alarmId;
    activeAlarm[alarmId].alarmType = alarmType;
    // tell that alarm is set
    activeAlarm[alarmId].isAlarmSet = 1;
    // Set this flag so that next time, it can be set again
    activeAlarm[alarmId].isAlarmTriggered = 0;
    activeAlarm[alarmId].duration = duration;

    // alarmType for Hour
    temp = currentTime.hour() + duration;
    activeAlarm[alarmId].endTimeHour = alarmType == 1 ? (temp > 23 ? (temp - 24) : temp) : currentTime.hour();

    // alarmType for Minute
    temp = currentTime.minute() + duration;
    activeAlarm[alarmId].endTimeMinute = alarmType == 2 ? (temp > 59 ? (temp - 60) : temp) : currentTime.minute();

    activeAlarm[alarmId].endTimeSecond = currentTime.second();
    // alarmId = 0 [bathroom bulb setup] and bulb is not glowing then only setup this
    if (alarmId == 0 && !isBathroomLedOn) {
        // Turn ON the bulb by making relay LOW
        turnBulb("ON", "bathroomBulb");
        isBathroomLedOn = true;
        actionMessageLogger("setAlarm :: Turn ON the bulb by making relay LOW");
    }

    // alarmId = 1 [window bulb setup] and bulb is not glowing then only setup this
    /* Condition - 
    *   1. Whenever switch is turned ON, the bulb turned ON for 5 minute. So if user want to turn ON the bulb again then he has to turn this ON again.
    *   2. Work normally when switch is turned OFF. Irrespective of time left, the bulb has to be turned OFF.
    */
    if (alarmId == 1) {
        turnBulb("ON", "kitchenBulb");
    }

    // actionMessageLogger("setAlarm :: Timer SET Completed");
}

void matchAlarm() {
    // Match condition
    for (int i=0; i < 2; i++) {
        if (currentTime.hour() == activeAlarm[i].endTimeHour && currentTime.minute() >= activeAlarm[i].endTimeMinute && currentTime.second() >= activeAlarm[i].endTimeSecond && !activeAlarm[i].isAlarmTriggered) {
            actionMessageLogger("matchAlarm :: Timer Matched - alarm triggered");
            activeAlarm[i].isAlarmSet = 0;
            activeAlarm[i].isAlarmTriggered = 1;

            // Bathroom bulb handler - Check if current time reached the Off Timer and LED still ON
            if (activeAlarm[i].alarmId == 0 && isBathroomLedOn) {
                // Check if bttn pressed for long stay
                if (digitalRead(iAmStillInToiletSwitch) == LOW || iAmStillInToilet) {
                    // Set the new timer for 1 minute
                    // int alarmId, int duration, int alarmType
                    setAlarm(0, bathroomLightOnDuration, 2);
                } else {
                    turnBulb("OFF", "bathroomBulb");
                    // Resetting the flags
                    isBathroomLedOn = false;
                }
            }

            // kitchenBulb handler - Check if current time reached the Off Timer and LED still ON
            if (activeAlarm[i].alarmId == 1) {
                turnBulb("OFF", "kitchenBulb");
            }
        }
    }
}

void kitchen_control() {
    // when kitchen switch pressed = ON
    if (digitalRead(windowBulbSwitch) == LOW && !isKitchenLedOn) {
        if (activeAlarm[1].isAlarmSet == 0) {
            Serial.println("button pressed ON, setting alarm");
            // int alarmId, int duration, int alarmType
            setAlarm(1, kitchenLightOnDuration, 2);
            isKitchenLedOn = true;
        }
    }

    // when kitchen switch pressed = OFF
    if (digitalRead(windowBulbSwitch) == HIGH && isKitchenLedOn) {
        Serial.println("button pressed OFF");
        unsetAlarm(1);
        isKitchenLedOn = false;
        turnBulb("OFF", "kitchenBulb");
        delay(100);
    } 
}

void manual_control() {
    if (digitalRead(windowBulbSwitch) == LOW && windowBulbSwitchState == LOW) {
        turnBulb("ON", "OutBulb");
        windowLedRelayState = HIGH;
        windowBulbSwitchState = HIGH;
    }

    if (digitalRead(windowBulbSwitch) == HIGH && windowBulbSwitchState == HIGH) {
        turnBulb("OFF", "OutBulb");
        windowLedRelayState = LOW;
        windowBulbSwitchState = LOW;
    }

    kitchen_control();

    bathRoomAutomaticControl();

    matchAlarm();
}

void bathRoomAutomaticControl() {
    // Bathroom Lighting hours starts from - 6 pm (18:00) till 7 am (07:00)
    if (currentTime.hour() >= activeHourStart || currentTime.hour() <= activeHourEnd) {
        motionSensorStatus = digitalRead(bathroomMotionSensor);
        // activities detected
        if (motionSensorStatus == HIGH) {
          // Timer is not set
          if (activeAlarm[0].isAlarmSet == 0) {
              // int alarmId, int duration, int alarmType
              setAlarm(0, bathroomLightOnDuration, 2);
          }
        }
    }
}

void setup() {
    // Debug console
    Serial.begin(9600);
    delay(100);

    // Initial setup
    pinMode(windowBulbRelay, OUTPUT);
    pinMode(windowBulbSwitch, INPUT_PULLUP);
    pinMode(iAmStillInToiletSwitch, INPUT_PULLUP);
  
    // Inside bathroom setup - make relay as output and sensor asinput
    pinMode(bathroomBulbRelay, OUTPUT);
    pinMode(bathroomMotionSensor, INPUT);

    // Setup the RTC mmodule
    rtcSetup();

    digitalWrite(windowBulbRelay, HIGH);
    digitalWrite(bathroomBulbRelay, HIGH);
    Serial.println("Setup :: Setup completed");
}

// handle the situation where bathroom light turn on without any event [most probalbly false current flow in relay]
void cleanUpCode() {
    // Check if bathroom bulb is ON without actually triggered by motion
    if (activeAlarm[0].isAlarmSet == 0 && digitalRead(bathroomBulbRelay) == LOW) {
        digitalWrite(windowBulbRelay, HIGH);
    }
}

void loop() {
    currentTime = rtc.now();

    // call all manual control i.e. switches, relay
    kitchen_control();
}
