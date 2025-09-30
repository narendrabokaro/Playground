#include "FS.h"
#include <LittleFS.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

static const int RXPin = D5, TXPin = D6;
static const uint32_t GPSBaud = 9600;// SoftwareSerial for the GPS module (using available pins on ESP8266)

// Create a TinyGPSPlus object
TinyGPSPlus gps;
SoftwareSerial gpsSerial(D6, D5); // The constructor now takes RX and TX pins

// Web server setup
ESP8266WebServer server(80);
const char* AP_SSID = "RC_Flight_Logger";
const char* AP_PASSWORD = "<Change password>"; // Set a password for your AP

// File object for handling the LittleFS file
File dataFile;

// Timing variables using millis()
unsigned long previousMillis = 0;
const long interval = 2000; // Log data every 2 seconds (2000 milliseconds)
const unsigned long LOGGING_DURATION = 300000; // Log for 5 min (300 seconds)
unsigned long startLoggingTime = 0;
bool loggingActive = false; // Flag to control logging
bool waitingForFix = false; // Flag to control waiting for initial fix

// Define the pin for the onboard status LED
#define STATUS_LED_PIN D4

// Variable for the fixed log filename
const char* logFileName = "/gps_log.csv";
const char* kmlFileName = "/flight_path.kml";

// HTML page for user interface
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>My RC Flight Logger</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <h1>RC Flight Logger</h1>
  <p>Status: <span id="status">Ready</span></p>
  <p>GPS Fix: <span id="gpsFix">No Fix</span></p>
  <p>
    <button onclick="sendCommand('START_LOG')">Start Log</button>
    <button onclick="sendCommand('GET_KML')">Get KML</button>
  </p>
<script>
function sendCommand(command) {
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "/" + command, true);
  xhttp.send();
}

setInterval(function() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var response = JSON.parse(this.responseText);
      document.getElementById("status").innerHTML = response.status;
      document.getElementById("gpsFix").innerHTML = response.gpsFix;
    }
  };
  xhttp.open("GET", "/status", true);
  xhttp.send();
}, 2000);
</script>
</body>
</html>
)rawliteral";

// Function to indicate success
void indicateSuccess() {
  digitalWrite(STATUS_LED_PIN, LOW);
}

// Function to indicate a problem (and ready for commands)
void indicateError() {
  digitalWrite(STATUS_LED_PIN, HIGH);
}

// Function to blink the LED to indicate it's ready
void blinkLED(int times, int delay_ms) {
  for (int i = 0; i < times; i++) {
    indicateSuccess();
    delay(delay_ms);
    indicateError();
    delay(delay_ms);
  }
}

// Function to initialize or reset the log file
void createNewFile() {
  Serial.println("Creating new file: " + String(logFileName));
  File file = LittleFS.open(logFileName, "w");
  if (file) {
    file.println("latitude,longitude");
    file.close();
    Serial.println("Header written successfully.");
  } else {
    Serial.println("Failed to create file for header.");
    indicateError();
  }
}

// Function to write data to the LittleFS file
void writeToCSV(String data) {
  if (loggingActive) {
    File file = LittleFS.open(logFileName, "a");
    if (file) {
      file.println(data);
      file.close();
      Serial.println("Data written to LittleFS: " + data);
    } else {
      Serial.println("Error opening " + String(logFileName));
      indicateError();
    }
  }
}

// Function to read CSV, generate KML, and serve via web server
void handleGetKml() {
  File csvFile = LittleFS.open(logFileName, "r");
  if (!csvFile) {
    server.send(404, "text/plain", "File not found");
    return;
  }

  String kmlContent = "";
  kmlContent += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n<Document>\n<name>Flight Track</name>\n<Style id=\"rc_flight_style\">\n<LineStyle>\n<color>ff0000ff</color>\n<width>4</width>\n</LineStyle>\n</Style>\n<Placemark>\n<name>Flight Path</name>\n<styleUrl>#rc_flight_style</styleUrl>\n<LineString>\n<extrude>1</extrude>\n<tessellate>1</tessellate>\n<altitudeMode>absolute</altitudeMode>\n<coordinates>\n";

  csvFile.readStringUntil('\n');
  while (csvFile.available()) {
    String line = csvFile.readStringUntil('\n');
    int commaIndex = line.indexOf(',');
    if (commaIndex != -1) {
      String lat = line.substring(0, commaIndex);
      String lon = line.substring(commaIndex + 1);
      kmlContent += lon + "," + lat + ",0\n";
    }
  }
  csvFile.close();

  kmlContent += "</coordinates>\n</LineString>\n</Placemark>\n</Document>\n</kml>\n";
  
  server.sendHeader("Content-Disposition", "attachment; filename=\"flight_path.kml\"");
  server.send(200, "application/vnd.google-earth.kml+xml", kmlContent);
}

// Function to handle the root URL "/"
void handleRoot() {
  server.send(200, "text/html", index_html);
}

// Function to handle the start log command
void handleStartLog() {
  if (!loggingActive && !waitingForFix) {
    waitingForFix = true;
    server.send(200, "text/plain", "Received START_LOG command. Waiting for GPS fix...");
  } else {
    server.send(200, "text/plain", "Logging is either active or already waiting for a fix.");
  }
}

// Function to handle the status request
void handleStatus() {
  String status = loggingActive ? "Logging" : "Ready";
  if (waitingForFix) status = "Waiting for Fix";

  String gpsFix = gps.location.isValid() ? "Fix" : "No Fix";
  
  String jsonResponse = "{\"status\":\"" + status + "\",\"gpsFix\":\"" + gpsFix + "\"}";
  server.send(200, "application/json", jsonResponse);
}

void setup() {
  Serial.begin(115200);
  pinMode(STATUS_LED_PIN, OUTPUT);
  indicateError();

  Serial.println("Setting up Access Point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  IPAddress ip = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(ip);

  blinkLED(5, 500);

  if (!LittleFS.begin()) {
    Serial.println("LittleFS Mount Failed, trying to format...");
    if (LittleFS.format()) {
      Serial.println("LittleFS formatted successfully!");
      if (!LittleFS.begin()) {
        Serial.println("LittleFS remount failed! Halting.");
        indicateError();
        while (true);
      }
    } else {
      Serial.println("LittleFS formatting failed! Halting.");
      indicateError();
      while (true);
    }
  } else {
    Serial.println("LittleFS mounted successfully.");
  }
  
  gpsSerial.begin(GPSBaud); // The SoftwareSerial begin() method no longer takes a config parameter
  Serial.println("GPS serial started.");

  server.on("/", handleRoot);
  server.on("/GET_KML", handleGetKml);
  server.on("/START_LOG", handleStartLog);
  server.on("/status", handleStatus);
  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  server.handleClient();
  
  unsigned long start = millis();

  while (millis() - start < 1000) {
      while (gpsSerial.available() > 0) {
        gps.encode(gpsSerial.read());
      }

      if (waitingForFix && gps.location.isUpdated()) {
          Serial.println("Initial GPS fix acquired! Starting log session.");
          createNewFile();
          startLoggingTime = millis();
          loggingActive = true;
          waitingForFix = false;
          indicateSuccess();
      }

      if (loggingActive && (millis() - startLoggingTime >= LOGGING_DURATION)) {
        loggingActive = false;
        Serial.println("Logging duration ended. Ready for file transfer.");
        indicateError();
      }

      if (loggingActive && gps.location.isUpdated()) {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
          previousMillis = currentMillis;
          String dataString = String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
          writeToCSV(dataString);
        }
      }
      
      if (waitingForFix) {
        static unsigned long lastBlink = 0;
        if (millis() - lastBlink > 150) {
          lastBlink = millis();
          digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
        }
      }
  }
}
