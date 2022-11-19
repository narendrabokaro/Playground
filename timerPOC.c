#include <TimeLib.h>

long int turnOffTimer = 0;
boolean isLedOn = false;
const int LED = 5;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(57600);
  pinMode(LED, OUTPUT);
}

void loop() {
   // Set timer for 2 min
    if (turnOffTimer == 0) {
        turnOffTimer = now() + 60;
        Serial.println("Timer set for ");
        Serial.println(turnOffTimer);
        digitalWrite(LED, HIGH);
    }
 
    if ( now() > turnOffTimer && !isLedOn) {
        Serial.println("Timer clock matched");
        digitalWrite(LED, LOW);
        isLedOn = true;
    }

    Serial.println("Timer set at ");
    Serial.println(turnOffTimer);
    
    Serial.println("Time now :");
    Serial.println(now());
    delay(500);
}
