#include "FS.h"
#include <LittleFS.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// SoftwareSerial for the GPS module (using available pins on ESP8266)
static const int GPS_RX_PIN = D6;
static const int GPS_TX_PIN = D5;
static const uint32_t GPSBaud = 9600;

// Timing variables using millis()
unsigned long previousMillis = 0;
const long interval = 2000; // Log data every 2 seconds (2000 milliseconds)

// 5-minute logging duration in milliseconds
const unsigned long LOGGING_DURATION = 2 * 60 * 1000;
unsigned long startLoggingTime = 0;

const char* currentKmlFileName = "/current_flight.kml";
const char* previousKmlFileName = "/previous_flight.kml";

// File logging status variables
bool loggingActive = false;
bool isInitialFixAcquired = false;

const char* ssid = "TP-LINK_91BB";
const char* password = "84676597";

// Create a TinyGPSPlus object and SoftwareSerial object
TinyGPSPlus gps;
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);

// Define a server on port 80
ESP8266WebServer server(80);

// HTML content for the web page
const char* HTML_CONTENT = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>ESP8266 GPS Tracker</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  body { font-family: Arial; text-align: center; }
  .button { background-color: #4CAF50; border: none; color: white; padding: 15px 32px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin: 4px 2px; cursor: pointer; border-radius: 8px; }
  .button2 { background-color: #f44336; }
  .button3 { background-color: #008CBA; }
  .button4 { background-color: #e7e7e7; color: black; }
  #status-message { font-size: 24px; font-weight: bold; margin-top: 20px; }
</style>
</head>
<body>
<h1>ESP8266 GPS Tracker</h1>
<p>
  <a href="/start"><button class="button">Start Logging</button></a>
  <a href="/stop"><button class="button button2">Stop Logging</button></a>
</p>
<p>
  <a href="/download_current"><button class="button button3">Download Current Flight</button></a>
  <a href="/download_previous"><button class="button button4">Download Previous Flight</button></a>
</p>
<div id="status-message">Fetching status...</div>

<script>
function getStatus() {
  fetch('/status')
    .then(response => response.json())
    .then(data => {
      const statusElement = document.getElementById('status-message');
      if (data.loggingActive) {
        statusElement.textContent = "Logging Active...";
        statusElement.style.color = "#4CAF50"; // Green
      } else {
        statusElement.textContent = "Logging Stopped.";
        statusElement.style.color = "#f44336"; // Red
      }
    })
    .catch(error => {
      console.error('Error fetching status:', error);
      document.getElementById('status-message').textContent = "Error fetching status.";
      document.getElementById('status-message').style.color = "gray";
    });
}

// Update the status every 2 seconds
setInterval(getStatus, 2000);

// Basic button feedback without full refresh
document.querySelectorAll('a[href="/start"], a[href="/stop"]').forEach(button => {
  button.addEventListener('click', (e) => {
    e.preventDefault();
    const url = e.currentTarget.getAttribute('href');
    fetch(url)
      .then(() => {
        // Force an immediate status update after button press
        setTimeout(getStatus, 100);
      });
  });
});
</script>
</body>
</html>
)rawliteral";


// Function prototypes for web handlers
void handleRoot();
void handleStartButton();
void handleStopButton();
void handleDownloadCurrent();
void handleDownloadPrevious();
void handleNotFound();

// Core logging functions
bool finalizeKmlFile();
void startLogging();
void stopLogging();

void handleStatus() {
  String json = "{";
  json += "\"loggingActive\": " + String(loggingActive ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

// Function to handle the root page ("/")
void handleRoot() {
  server.send(200, "text/html", HTML_CONTENT);
}

bool finalizeKmlFile() {
  File file = LittleFS.open(currentKmlFileName, "a");
  if (!file) {
    Serial.println("Failed to open file for finalizing.");
    return false;
  }
  const String kmlFooter = "</coordinates>\n</LineString>\n</Placemark>\n</Document>\n</kml>\n";
  file.print(kmlFooter);
  file.close();
  Serial.println("KML file finalized.");
  return true;
}

void startLogging() {
  if (loggingActive) return;

  if (LittleFS.exists(currentKmlFileName)) {
    Serial.println("Finalizing previous unsaved current flight.");
    finalizeKmlFile();
    if (LittleFS.exists(previousKmlFileName)) {
      LittleFS.remove(previousKmlFileName);
    }
    LittleFS.rename(currentKmlFileName, previousKmlFileName);
  }

  Serial.println("Starting new KML log.");
  File file = LittleFS.open(currentKmlFileName, "w");
  if (!file) {
    Serial.println("Failed to create file for KML header.");
    return;
  }
  const String kmlHeader = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n<Document>\n<name>Flight Track</name>\n<Style id=\"rc_flight_style\">\n<LineStyle>\n<color>ff0000ff</color>\n<width>4</width>\n</LineStyle>\n</Style>\n<Placemark>\n<name>Flight Path</name>\n<styleUrl>#rc_flight_style</styleUrl>\n<LineString>\n<extrude>1</extrude>\n<tessellate>1</tessellate>\n<altitudeMode>absolute</altitudeMode>\n<coordinates>\n";
  file.print(kmlHeader);
  file.close();

  loggingActive = true;
  previousMillis = millis();
  startLoggingTime = millis(); // Store the time when logging starts
  Serial.println("Logging active.");
}

void stopLogging() {
  if (!loggingActive) return;

  finalizeKmlFile();
  loggingActive = false;
  Serial.println("Logging stopped.");
}

void handleStartButton() {
  startLogging();
  server.send(200, "text/plain", "Start Button action acknowledged.");
}

void handleStopButton() {
  stopLogging();
  server.send(200, "text/plain", "Stop Button action acknowledged.");
}

void handleDownloadCurrent() {
  if (loggingActive) {
    server.send(409, "text/plain", "Error: Cannot download current file while logging is active. Stop logging first.");
    return;
  }
  if (LittleFS.exists(currentKmlFileName)) {
    Serial.println("Streaming current KML file.");
    File file = LittleFS.open(currentKmlFileName, "r");
    if (file) {
      // Add the Content-Disposition header to suggest a filename for the download
      server.sendHeader("Content-Disposition", "attachment; filename=\"current_flight.kml\"");
      server.streamFile(file, "application/vnd.google-earth.kml+xml");
      file.close();
    } else {
      server.send(500, "text/plain", "Error opening file.");
    }
  } else {
    server.send(404, "text/plain", "File not found.");
  }
}

void handleDownloadPrevious() {
  if (LittleFS.exists(previousKmlFileName)) {
    Serial.println("Streaming previous KML file.");
    File file = LittleFS.open(previousKmlFileName, "r");
    if (file) {
      // Add the Content-Disposition header for the previous flight file
      server.sendHeader("Content-Disposition", "attachment; filename=\"previous_flight.kml\"");
      server.streamFile(file, "application/vnd.google-earth.kml+xml");
      file.close();
    } else {
      server.send(500, "text/plain", "Error opening file.");
    }
  } else {
    server.send(404, "text/plain", "File not found.");
  }
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not found");
}

void waitForFix() {
  Serial.println("Waiting for GPS fix...");
  while (!isInitialFixAcquired) {
    server.handleClient();
    while (gpsSerial.available() > 0) {
      if (gps.encode(gpsSerial.read())) {
        if (gps.location.isValid()) {
          isInitialFixAcquired = true;
          Serial.println("GPS fix acquired! Starting logging.");
          break;
        }
      }
    }
    if (!isInitialFixAcquired) {
      Serial.print(".");
      delay(500);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (!LittleFS.begin()) {
    Serial.println("LittleFS Mount Failed. Formatting...");
    if (LittleFS.format()) {
      Serial.println("LittleFS formatted successfully.");
      LittleFS.begin();
    } else {
      Serial.println("LittleFS formatting failed. Halting.");
      while (true);
    }
  } else {
    Serial.println("LittleFS mounted successfully.");
  }

  gpsSerial.begin(GPSBaud);
  Serial.println("GPS serial started.");

  server.on("/", handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/start", HTTP_GET, handleStartButton);
  server.on("/stop", HTTP_GET, handleStopButton);
  server.on("/download_current", HTTP_GET, handleDownloadCurrent);
  server.on("/download_previous", HTTP_GET, handleDownloadPrevious);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

  waitForFix();
  startLogging();
}

void loop() {
  server.handleClient();

  // Check if logging has exceeded the duration
  if (loggingActive && (millis() - startLoggingTime >= LOGGING_DURATION)) {
    Serial.println("5-minute logging duration reached. Stopping logging automatically.");
    stopLogging();
  }

  if (loggingActive) {
    while (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
    }
    if (millis() - previousMillis >= interval) {
      previousMillis = millis();
      if (gps.location.isValid() && gps.location.isUpdated()) {
        String dataString = String(gps.location.lng(), 6) + "," + String(gps.location.lat(), 6) + "," + String(gps.altitude.meters(), 2) + "\n";
        File file = LittleFS.open(currentKmlFileName, "a");
        if (file) {
          file.print(dataString);
          file.close();
        }
      }
    }
  }
}
