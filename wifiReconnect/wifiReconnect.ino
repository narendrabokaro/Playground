/*************************************************************
  Filename - wifiReconnect
  Description - Create a wifi connection and it will check the internet connection after every 5 minute and keep trying to next 10 seconds before moving to 
  pick the next instruction from loop()
 *************************************************************/
#include <ESP8266WiFi.h>

const char *ssid = "wifi name";
const char *password = "your password";

void checkConnection() {
    unsigned long timeout = millis();
    Serial.println("Checking for connection...");

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("No connection found, reconnecting for 10 seconds ");
        WiFi.begin(ssid, password);
        
        while (WiFi.status() != WL_CONNECTED && millis() - timeout < 10000) {
            delay(500);
            Serial.print(".");
        }
    } else {
        Serial.println("Healthy internet connection found");
    }
}

// Try to reconnect after every minute
const unsigned long connCheckTimeOut = 5*60000; // 5 min
unsigned long lastConnCheckTime;

void setup()
{
    Serial.begin(115200);
    delay(10);

    Serial.println();
    Serial.println();
    Serial.println("Connecting to ");
    Serial.println(ssid);
    unsigned long timeout = millis();
    
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED && millis() - timeout < 10000) {
        delay(1000);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.println("Wifi Connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("Internet is not available at the moment.");  
    }
}

void loop() {
    unsigned long now = millis();

    // Check internet connection after every 5 minute 
    if (now - lastConnCheckTime >= connCheckTimeOut) {
        lastConnCheckTime = now;
        checkConnection();
    }
}
