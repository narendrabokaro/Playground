
#include <ThreeWire.h>  
#include <RtcDS1302.h>
#define countof(a) (sizeof(a) / sizeof(a[0]))

int alarmHour = 0;
int alarmMinute = 0;
int alarmSecond = 0;

// Flag indicates alarm status - enabled/disabled
int isAlarmSet = 0;
// Flag indicate whether alarm triggered or not
int isAlarmTriggered = 0;

// GPIO pin
ThreeWire myWire(D4,D5,D2);
RtcDS1302<ThreeWire> Rtc(myWire);

void printDateTime(const RtcDateTime& dt){
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.print(datestring);
}

void rtcSetup() {
    Rtc.Begin();
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

    // Set the time if not already configured
    if (!Rtc.IsDateTimeValid()) {
        Serial.println("RTC lost confidence in the DateTime, so setting the system time");
        Rtc.SetDateTime(compiled);
    }

    if (Rtc.GetIsWriteProtected()) {
        Serial.println("RTC was write protected, enabling writing now");
        Rtc.SetIsWriteProtected(false);
    }

    if (!Rtc.GetIsRunning()) {
        Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
    }

    // Get a copy of current date and time
    RtcDateTime now = Rtc.GetDateTime();

    if (now < compiled) {
        Serial.println("RTC is older than compile time so updating DateTime)");
        Rtc.SetDateTime(compiled);
    } else if (now > compiled) {
        Serial.println("RTC is newer than compile time. (this is expected)");
    } else if (now == compiled) {
        Serial.println("RTC is the same as compile time! (not expected but all is fine)");
    }
}

// duration = for how long you want to set alarm
// alarmType = 1 for hour | 2 for Minuate
void setAlarm(int duration, int alarmType, RtcDateTime currentTime) {
    int temp = 0;

    Serial.println("Timer SET");
    alarmHour = currentTime.Hour();
    alarmMinute = currentTime.Minute();
    alarmSecond = currentTime.Second();

    // alarmType for Hour
    if (alarmType == 1) {
        // tell that alarm is set
        isAlarmSet = 1;
        // Set this flag so that next time, it can be set again
        isAlarmTriggered = 0;
        temp = currentTime.Hour() + duration;
        alarmHour = temp > 23 ? (temp - 24) : temp;
        digitalWrite(D7, HIGH);
    }

    // alarmType for Minuate
    if (alarmType == 2) {
        // tell that alarm is set
        isAlarmSet = 1;
        // Set this flag so that next time, it can be set again
        isAlarmTriggered = 0;
        temp = currentTime.Minute() + duration;
        alarmMinute = temp > 59 ? (temp - 60) : temp;
        digitalWrite(D7, HIGH);
    }

    Serial.println("alarmHour: ");
    Serial.println(alarmHour);
    Serial.println("alarmMinute: ");
    Serial.println(alarmMinute);
    Serial.println("alarmSecond: ");
    Serial.println(alarmSecond);
}

void matchAlarm(RtcDateTime currentTime) {
    // Match condition
    if (currentTime.Hour() == alarmHour && currentTime.Minute() == alarmMinute && currentTime.Second() == alarmSecond && !isAlarmTriggered) {
        // Timer set - alarm triggered
        Serial.println("Timer Matched - alarm triggered");
        digitalWrite(D7, LOW);
        isAlarmTriggered = 1;
    }  
}

void setup () 
{
    Serial.begin(57600);
    pinMode(D1, INPUT_PULLUP);
    pinMode(D7, OUTPUT);
    // Call to setup the RTC mmodule
    rtcSetup();
}

void loop () {
    RtcDateTime now = Rtc.GetDateTime();

    // Set the timerAlert
    printDateTime(now);
    Serial.println();

    if (!now.IsValid())
    {
        // Common Causes: the battery on the device is low or even missing and the power line was disconnected
        Serial.println("RTC lost confidence in the DateTime!");
    }

    // Setting the alarm for 1 minuate
    if (digitalRead(D1) == LOW) {
        // duration, alarmType, currentTime
        setAlarm(2, 2, now);
    }
    
    matchAlarm(now);

    delay(1000); // One second
}
