#include <LittleFS.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>

// PIN CONFIGURATION
#define GPS_TX_PIN 12   // D6
#define GPS_RX_PIN 13   // D7
#define LED_PIN 14      // D5
#define SWITCH_PIN 0    // D3
#define SDA_PIN 4       // D2
#define SCL_PIN 5       // D1

TinyGPSPlus gps;
SoftwareSerial ss(GPS_TX_PIN, GPS_RX_PIN);
Adafruit_BMP280 bmp;

File logFile;
bool isLogging = false;
float altBaseline = 0; // Takeoff altitude reference
unsigned long lastLogTime = 0;

void setup() {
  Serial.begin(115200);
  ss.begin(9600);
  Wire.begin(SDA_PIN, SCL_PIN);
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);

  if (!LittleFS.begin()) {
    Serial.println("LittleFS Mount Failed");
    return;
  }
  
  if (!bmp.begin(0x76)) { // Change to 0x77 if 0x76 fails
    Serial.println("BMP280 not found");
  }
}

void loop() {
  while (ss.available() > 0) gps.encode(ss.read());

  bool switchOn = (digitalRead(SWITCH_PIN) == LOW);
  bool hasFix = (gps.location.isValid() && gps.satellites.value() >= 4);

  // LED STATUS INDICATORS
  if (!hasFix) {
    digitalWrite(LED_PIN, (millis() / 200) % 2); // FAST FLASH: Searching
  } else if (isLogging) {
    digitalWrite(LED_PIN, (millis() / 1000) % 2); // SLOW BLINK: Recording
  } else {
    digitalWrite(LED_PIN, HIGH); // SOLID: Ready to record
  }

  // START RECORDING (Switch flipped ON + GPS Fix)
  if (switchOn && !isLogging && hasFix && gps.date.isValid()) {
    char path[30];
    sprintf(path, "/%02d%02d_%02d%02d.kml", gps.date.day(), gps.date.month(), gps.time.hour(), gps.time.minute());
    
    logFile = LittleFS.open(path, "w");
    if (logFile) {
      // KML Header
      logFile.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
      logFile.println("<kml xmlns=\"http://www.opengis.net\"><Document><Placemark><name>Flight Path</name>");
      logFile.println("<LineString><extrude>1</extrude><altitudeMode>relativeToGround</altitudeMode><coordinates>");
      
      // Auto-Zero: Capture ground altitude as baseline
      altBaseline = bmp.readAltitude(1013.25); 
      isLogging = true;
      Serial.print("Recording to: "); Serial.println(path);
    }
  }

  // LOG DATA EVERY 2 SECONDS
  if (isLogging && millis() - lastLogTime >= 2000) {
    lastLogTime = millis();
    
    // Relative Altitude Calculation
    float currentAlt = bmp.readAltitude(1013.25) - altBaseline;

    // KML format: longitude,latitude,altitude
    logFile.print(gps.location.lng(), 6);
    logFile.print(",");
    logFile.print(gps.location.lat(), 6);
    logFile.print(",");
    logFile.println(currentAlt, 1);
    logFile.flush(); 
  }

  // STOP RECORDING (Switch flipped OFF)
  if (!switchOn && isLogging) {
    logFile.println("</coordinates></LineString></Placemark></Document></kml>");
    logFile.close();
    isLogging = false;
    Serial.println("Flight Saved.");
  }
}
