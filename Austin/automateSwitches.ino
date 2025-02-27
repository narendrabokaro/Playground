/*
* Basic automation requirement - provide one Blynk app switch to turn ON/OFF the corner lamp
* Pending - 
* 1. boundary cases failing [--:--]
* 2. If auto mode selected then only set the alarm
* 3. Dont allow to alarm for pass time
* 4. create a flag to keep that fresh time has been selected
*/

#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL2dIz_IQKc"
#define BLYNK_TEMPLATE_NAME "austinHomeProject"
#define BLYNK_AUTH_TOKEN "h8Nh69YR2RpXDDD-ydl48xtA0_jhl4jc"

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
char pass[] = "goofycountry240";

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

void timeMatchToSwitchOffLight() {
    timeClient.update();
    int currentHour = timeClient.getHours();
    int currentMinute = timeClient.getMinutes();

    Serial.print("timeMatchToSwitchOffLight Func :");
    Serial.print(currentHour);
    Serial.println(currentMinute);

    // If autoMode Button pressed + current time matches with target time and bulb is on
    if (autoModeButtonState && (currentHour == whenToSwitchOffLightHour && currentMinute >= whenToSwitchOffLightMinute) && isLightTurnedOn) {
        Serial.print("timeMatchToSwitchOffLight : Condition matched");
        // Turn OFF the light
        ledEvent(false);
    }
}

void timeMatch() {
    timeClient.update();
    int currentHour = timeClient.getHours();
    int currentMinute = timeClient.getMinutes();

    Serial.print("TimeMatch Func :");
    Serial.print(currentHour);
    Serial.println(currentMinute);

    // If autoMode Button pressed + current time matches with target time and bulb is off
    if (autoModeButtonState && (currentHour == targetHour && currentMinute >= targetMinute) && !isLightTurnedOn) {
        Serial.print("timeMatch inside");
        // Turn ON the light ON
        ledEvent(true);
    }
}

void enableAutoMateAlarm (int &autoMateBttnTimerID) {
  Serial.print("Alarm enabled :");
  Serial.print(targetHour);
  Serial.print(targetMinute);
  // Set the timer to execute timeMatch() every minute
  autoMateBttnTimerID = timer.setInterval(60000L, timeMatch);
}

void disableAutoMateAlarm (int &autoMateBttnTimerID, BlynkTimer &timer) {
  Serial.print("Alarm disabled");
  timer.disable(autoMateBttnTimerID);
}

// Timer to check when to switch off the light
void setLedOffAlarm(int &turnOffLedTimerID) {
  // set a new alarm
  // Set the timer to execute timeMatch() every minute
  turnOffLedTimerID = timer.setInterval(60000L, timeMatchToSwitchOffLight);
}

// Remove the above timer when auto_mate_btn is dis-engage
void unsetLedOffAlarm(int &turnOffLedTimerID, BlynkTimer &timer) {
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
    enableAutoMateAlarm(autoMateBttnTimerID);
    // set the second timer to turn the LED off @ 7.00 AM next morning
    setLedOffAlarm(turnOffLedTimerID);
  } else {
    // Disable the timer
    Serial.println("Disabled the timer");
    // unset the first timer based on time choosen to turn the LED on
    disableAutoMateAlarm(autoMateBttnTimerID, timer);
    // unset the second timer to turn the LED off @ 7.00 AM next morning
    unsetLedOffAlarm(turnOffLedTimerID, timer);
    // Switch off the buld
    ledEvent(false);
  }
}

// Method called when user set the auto start time
// And then user has to press the AutoMode button from mobile
BLYNK_WRITE(SET_AUTO_START_TIME) {
  if (param[0].asInt() != 0) {
      assignUserTimeFromTimerCompoent(param[0].asInt(), targetHour, targetMinute);
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
