/*************************************************************
  Filename - nodeIrrigationProject
  Description - Home garden watering system which turn ON daily once and water the plant. To conserve the Battery, it will go to deep sleep after evrey one hour. And during wake up
  time, it will check the clock and if its matches then it will water the plant and go for deep sleep.
  SPECIAL INSTRUCTION - Pin D0 connected to RST pin in order to perform Deep Sleep -> wake up operation [Remove this while programing the MCU]
  version - 1.0.1
  type - Node version
  Updates/ Fixes -
  Added a buzzer which beeps just before pump starts
  Changed the motor running duration for 45 seconds
  
  Debug instructions -
  1> Always check the active hours.
 *************************************************************/
// For RTC module (DS1307)
#include "RTClib.h"

// D1 and D2 allotted to RTC module DS1307
RTC_DS1307 rtc;
DateTime currentTime;

#define buzzerPin D3
#define motorRelay D5

// Time for various comparision
struct Time {
    int hours;
    int minutes;
};

// Describe the time for which motor run and water the plant (In Seconds) - 45 Seconds
unsigned int motorRunningTime = 45000;

// Garden Active hours - Morning 10.10 AM to 11.10 AM
struct Time activeHourStartTime = {10, 10};
struct Time activeHourEndTime = {11, 10};

// Indicate (boolean) if time if greater/less than given time
bool diffBtwTimePeriod(struct Time start, struct Time end) {
   while (end.minutes > start.minutes) {
      --start.hours;
      start.minutes += 60;
   }

   return (start.hours - end.hours) >= 0;
}

// Return true if current time falls between active hours
bool isActiveHours() {
    return diffBtwTimePeriod({currentTime.hour(), currentTime.minute()}, activeHourStartTime) && diffBtwTimePeriod(activeHourEndTime, {currentTime.hour(), currentTime.minute()});
}

void lookNPlantWater() {

    if (isActiveHours()) {
        // Make one short beep
        digitalWrite(buzzerPin, HIGH);
        delay(500);
        digitalWrite(buzzerPin, LOW);
        delay(500);

        Serial.print("Running the pump for specified time = 45 second");
        digitalWrite(motorRelay, HIGH);
        delay(motorRunningTime);
        digitalWrite(motorRelay, LOW);
        delay(1000);
    }
}

// For RTC module setup
void rtcSetup() {
    Serial.println("rtcSetup :: Health status check");
    delay(1000);

    if (! rtc.begin()) {
        Serial.println("rtcSetup :: Couldn't find RTC");
        Serial.flush();
        while (1) delay(10);
    }

    if (! rtc.isrunning()) {
        Serial.println("rtcSetup :: RTC is NOT running, Please uncomment below lines to set the time!");
        // When time needs to be set on a new device, or after a power loss, the
        // following line sets the RTC to the date & time this sketch was compiled
        //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        
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

void setup() {
  Serial.begin(115200);
  pinMode(motorRelay, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  // Setup the RTC mmodule
  rtcSetup();

  delay(1000);

  // Look and plant the water at schedule time
  lookNPlantWater();

  Serial.println("I'm awake, but I'm going into deep sleep mode for 1 hour");
  // Deep sleep time - 3600 Second ~ 1 Hour
  ESP.deepSleep(3600e6);
}

void loop() {}