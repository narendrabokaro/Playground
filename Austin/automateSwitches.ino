/*
* Basic automation requirement - provide one Blynk app switch to turn ON/OFF the corner lamp
* Pending - 
* 1. boundary cases failing [--:--]
* 2. If auto mode selected then only set the alarm
*/

#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL2dIz_IQKc"
#define BLYNK_TEMPLATE_NAME "austinHomeProject"
#define BLYNK_AUTH_TOKEN "<xxxxxxxxx>-ydl48xtA0_jhl4jc"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// virtual pins for bulb
#define VPIN_BUTTON_1 V0
#define AUTO_MODE_BTN V1
// datastream available only for mobile template
#define SET_AUTO_START_TIME V6
#define ledLightRelaySwitch D1

// Function creates the timer object
BlynkTimer timer;
int autoMateBttnTimerID;

// WiFi credentials.
char ssid[] = "Spectrum326";
char pass[] = "<password>";

// Default - Off
bool roomLightSwtich = LOW;

// Auto Button Default state
bool isLightTurnedOn = false;
bool autoButtonState = LOW;

// Time for turning ON the light [Arming/ Disarming]
unsigned int targetHour;
unsigned int targetMinute;


// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void assignUserTimeFromTimerCompoent(const uint32_t seconds, unsigned int &targetHour, unsigned int &targetMinute) {
    uint32_t t = seconds;
    uint8_t ss;

    ss = t % 60;

    t = (t - ss)/60;
    targetMinute = t % 60;

    t = (t - targetMinute)/60;
    targetHour = t;

    Serial.print("Hour : ");
    Serial.println(targetHour);

    Serial.print("Min : ");
    Serial.println(targetMinute);
}

// Method full dedicated to turning LED on/off
void ledEvent(bool turnedOn) {
    if (turnedOn) {
      // Turn the light ON
      // TODO : make conditional
      Serial.print("Turn on the light");
      // ESP LED trurned ON
      digitalWrite(LED_BUILTIN, LOW);

      // Actual LED switched ON by making relay high
      digitalWrite(ledLightRelaySwitch, HIGH);
      // Update the central flag to denote led condition
      isLightTurnedOn = true;
    } else {
      // Turn the light Off
      // TODO : make conditional
      Serial.print("Turn off the light");
      // ESP LED trurned Off
      digitalWrite(LED_BUILTIN, HIGH);

      // Actual LED switched Off by making relay low
      digitalWrite(ledLightRelaySwitch, LOW);
      // Update the central flag to denote led condition
      isLightTurnedOn = false;
    }
}

void alarmSetFunc() {
    timeClient.update();

    int currentHour = timeClient.getHours();
    int currentMinute = timeClient.getMinutes();
    
    // If current time matches with target time and bulb is off
    if ((currentHour == targetHour && currentMinute >= targetMinute) && !isLightTurnedOn) {
        // Turn ON the light ON
        ledEvent(true);
    }
}

// Use this to turn the LED off daily [Switch off the bulb at 7 AM everyday]
// NOt being Used
void myTimer() {
    timeClient.update();

    // Hour and Minute data
    int currentHour = timeClient.getHours();
    Serial.print("Hour: Minute = ");
    Serial.println(currentHour);
    int currentMinute = timeClient.getMinutes();
    Serial.println(" : ");
    Serial.println(currentMinute);

    // If bulb ON
    if ((currentHour >= targetHour && currentMinute >= targetMinute) && !isLightTurnedOn) {
      Serial.print("Turn on the light");
      digitalWrite(LED_BUILTIN, LOW);  // Turn the LED on
      // light switched ON by making relay high
      digitalWrite(ledLightRelaySwitch, HIGH);
      isLightTurnedOn = true;
    }
}

BLYNK_CONNECTED() {
  // Request the latest state from the server
  Blynk.syncVirtual(VPIN_BUTTON_1);
  Blynk.syncVirtual(AUTO_MODE_BTN);
  Blynk.syncVirtual(SET_AUTO_START_TIME);
  Serial.println("Blynk connected Method");
}

// When App button is pushed - switch the state
BLYNK_WRITE(VPIN_BUTTON_1) {
  roomLightSwtich = param.asInt();
  if (roomLightSwtich == 1) {
    Serial.println("Led ON");
    digitalWrite(LED_BUILTIN, LOW);  // Turn the LED on
    // light switched ON by making relay high
    digitalWrite(ledLightRelaySwitch, HIGH);
    isLightTurnedOn = true;
  } else { 
    Serial.println("Led Off");
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off
    // light switched off by making relay low
    digitalWrite(ledLightRelaySwitch, LOW);
    isLightTurnedOn = false;
  }
}

// For Auto Mode button - it will automatically turn on LED on certain time [Arm/ disarm]
BLYNK_WRITE(AUTO_MODE_BTN) {
  autoButtonState = param.asInt();

  // Button is pressed ON
  if (autoButtonState == 1) {
    // Enable the timer
    Serial.println("Enable the timer");
    timer.enable(autoMateBttnTimerID);
    Serial.print("Hour : ");
    Serial.println(targetHour);

    Serial.print("Min : ");
    Serial.println(targetMinute);
  } else {
    // Disable the timer
    Serial.println("Disable the timer");
    timer.disable(autoMateBttnTimerID);
  }
}

// Method called when user set the auto start time
// And then user has to press the AutoMode button from mobile
BLYNK_WRITE(SET_AUTO_START_TIME) {
  assignUserTimeFromTimerCompoent(param[0].asInt(), targetHour, targetMinute);
}

// When App button is pushed - show the latest time on label
// BLYNK_WRITE(AUTO_MODE_BTN) {
//   timeClient.update();

//   String formattedTime = timeClient.getFormattedTime();
//   Serial.print("Formatted Time: ");
//   Serial.println(formattedTime);
//   Blynk.virtualWrite(V2, formattedTime);

//   // Hour and Minute data
//   // int currentHour = timeClient.getHours();
//   // Serial.print("Hour: ");
//   // Serial.println(currentHour);

//   // int currentMinute = timeClient.getMinutes();
//   // Serial.print("Minutes: ");
//   // Serial.println(currentMinute);
// }

void setup() {
  // Debug console
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ledLightRelaySwitch, OUTPUT);
  digitalWrite(ledLightRelaySwitch, HIGH);

  // Blynk configuration
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Connect to internet
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Initialize a NTPClient to get time
  timeClient.begin();
  // Austin timezone offset = GMT - 6
  timeClient.setTimeOffset(-21600);

  // Execute the myTimer() after every minute and disabled on start up
  autoMateBttnTimerID = timer.setInterval(60000L, alarmSetFunc);
  timer.disable(autoMateBttnTimerID);
}

void loop() {
  // Runs all Blynk stuff
  Blynk.run();
  // runs BlynkTimer
  timer.run();
}
