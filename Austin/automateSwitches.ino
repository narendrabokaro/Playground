/*
* Basic automation requirement - provide one Blynk app switch to turn ON/OFF the corner lamp
* Pending - 
* 1. boundary cases failing [--:--]
* 2. If auto mode selected then only set the alarm
* 3. Dont allow to alarm for pass time
* 4. create a flag to keep that fresh time has been selected
* 5. timeMatchToSwitchOffLight() - turn off the Led + Remove this alarm + 
*/

#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL2dIz_IQKc"
#define BLYNK_TEMPLATE_NAME "austinHomeProject"
#define BLYNK_AUTH_TOKEN "<fdasfaDSFAsasfas>-ydl48xtA0_jhl4jc"

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

// WiFi credentials.
char ssid[] = "Spectrum326";
char pass[] = "password";

// Function creates the timer object
BlynkTimer timer;
// Set the timer to switch on the light at user given time
int autoMateBttnTimerID;
// System will set this timer based on above timer to turn off the light at 7.10 AM
int turnOffLedTimerID;

// Default - Off
bool roomLightSwtich = LOW;
// Auto Button Default state
bool isLightTurnedOn = false;
int autoModeButtonState = 0;

// Turn off the light at morning 7.10 am
int whenToSwitchOffLightHour = 7;
int whenToSwitchOffLightMinute = 10;

// Time for turning ON the light [Arming/ Disarming]
uint32_t targetStartTime;
uint32_t targetEndTime;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

/*
* Description ledEvent
* Method full dedicated to turning LED on/off
*/
void ledEvent(bool turnedOn) {
    if (turnedOn) {
      // Turn the light ON
      // TODO : make conditional
      Serial.println("Turn on the light");
      // ESP LED trurned ON
      digitalWrite(LED_BUILTIN, LOW);

      // Actual LED switched ON by making relay high
      digitalWrite(ledLightRelaySwitch, HIGH);
      // Update the central flag to denote led condition
      isLightTurnedOn = true;
    } else {
      // Turn the light Off
      // TODO : make conditional
      Serial.println("Turn off the light");
      // ESP LED trurned Off
      digitalWrite(LED_BUILTIN, HIGH);

      // Actual LED switched Off by making relay low
      digitalWrite(ledLightRelaySwitch, LOW);
      // Update the central flag to denote led condition
      isLightTurnedOn = false;
    }
}

/*
* Description timeMatchToSwitchOnLight
* Description Keep checking for time match when to turn off
*/
void timeMatchToSwitchOffLight() {
    timeClient.update();
    uint32_t currentTime;
    uint32_t whenToSwitchOffLightTime;

    // Fetch the hour and minute from timeClient
    int currentHour = timeClient.getHours();
    int currentMinute = timeClient.getMinutes();
    // Convert current time to seconds
    currentTime = currentHour * 3600 + currentMinute * 60;
    // Convert the time to switch off light
    whenToSwitchOffLightTime = whenToSwitchOffLightHour * 3600 + whenToSwitchOffLightMinute * 60;

    Serial.println("Swtich Off Timer EpochTime :");
    Serial.print(currentTime);
    Serial.println("Timer will Switch Off Light @ ");
    Serial.print(whenToSwitchOffLightTime);

    // If autoMode Button pressed + current time matches with target time and bulb is on
    if (autoModeButtonState && isLightTurnedOn && currentTime >= whenToSwitchOffLightTime) {
        Serial.println("whenToSwitchOffLightTime : Condition matched");
        // Turn OFF the light
        ledEvent(false);
    }
}

/*
* Description timeMatchToSwitchOnLight
* Run a function every minute to match the time for turning the light on
*/
void timeMatchToSwitchOnLight() {
    timeClient.update();
    uint32_t currentTime;

    // Fetch the hour and minute from timeClient
    int currentHour = timeClient.getHours();
    int currentMinute = timeClient.getMinutes();
    // Convert current time to seconds
    currentTime = currentHour * 3600 + currentMinute * 60;

    Serial.println("current EpochTime :");
    Serial.print(currentTime);
    Serial.println("Timer will Switch On Light @ ");
    Serial.print(targetEndTime);

    // If autoMode Button pressed + bulb is off + current time matches with target time
    if (autoModeButtonState && !isLightTurnedOn && (currentTime >= targetStartTime && currentTime <= targetEndTime)) {
        Serial.println("timeMatchToSwitchOnLight timer match");
        // Turn ON the light ON
        ledEvent(true);
    }
}

/*
* Description setLedOnAlarm
* Run a function every minute to match the time for turning the light on
*/
void setLedOnAlarm (int &autoMateBttnTimerID) {
  Serial.println("setLedOnAlarm enabled :");
  // Set the timer to execute timeMatchToSwitchOnLight() every minute
  autoMateBttnTimerID = timer.setInterval(60000L, timeMatchToSwitchOnLight);
}

/*
* Description unsetLedOnAlarm
* Remove the above alarm
*/
void unsetLedOnAlarm (int &autoMateBttnTimerID, BlynkTimer &timer) {
  Serial.println("unsetLedOnAlarm disabled");
  timer.disable(autoMateBttnTimerID);
}

// Timer to check when to switch off the light
void setLedOffAlarm(int &turnOffLedTimerID) {
  Serial.println("setLedOffAlarm enabled :");
  // set a new alarm
  // Set the timer to execute timeMatchToSwitchOffLight() every minute
  turnOffLedTimerID = timer.setInterval(60000L, timeMatchToSwitchOffLight);
}

// Remove the above timer when auto_mate_btn is dis-engage
void unsetLedOffAlarm(int &turnOffLedTimerID, BlynkTimer &timer) {
  Serial.println("unsetLedOffAlarm enabled :");
  // unset above alarm
  timer.disable(turnOffLedTimerID);
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
    // Turn ON the light
    ledEvent(true);
  } else { 
    // Turn OFF the light
    ledEvent(false);
  }
}

// For Auto Mode button - it will automatically turn on LED on certain time [Arm/ disarm]
BLYNK_WRITE(AUTO_MODE_BTN) {
  autoModeButtonState = param.asInt();

  // Button is pressed ON
  if (autoModeButtonState == 1) {
    // Enable the timer
    Serial.println("Enabled the timer");
    // set the first timer based on time choosen to turn the LED on
    setLedOnAlarm(autoMateBttnTimerID);
    // set the second timer to turn the LED off @ 7.00 AM next morning
    setLedOffAlarm(turnOffLedTimerID);
  } else {
    // Disable the timer
    Serial.println("Disabled the timer");
    // unset the first timer based on time choosen to turn the LED on
    unsetLedOnAlarm(autoMateBttnTimerID, timer);
    // unset the second timer to turn the LED off @ 7.00 AM next morning
    unsetLedOffAlarm(turnOffLedTimerID, timer);
    // Switch off the buld
    ledEvent(false);
  }
}

// Method being called when user set the time component (HH:MM)
// And then user has to press the AutoMode button from mobile
BLYNK_WRITE(SET_AUTO_START_TIME) {
  uint32_t inSeconds = param[0].asInt();

  if (inSeconds != 0) {
      targetStartTime = inSeconds;
      Serial.println("Start timer: ");
      Serial.print(targetStartTime);
      // Set the target end time - 10 minute later
      targetEndTime = inSeconds + 600;
      Serial.println("End timer: ");
      Serial.print(targetEndTime);
  }
}

void setup() {
  // Debug console
  Serial.begin(9600);
  // Turn Off LED built in on ESP board
  pinMode(LED_BUILTIN, OUTPUT);
  // Mark ledLightRelaySwitch as O/P device
  pinMode(ledLightRelaySwitch, OUTPUT);
  // Turn Off the Led
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
}

void loop() {
  // Runs all Blynk stuff
  Blynk.run();
  // runs BlynkTimer
  timer.run();
}
