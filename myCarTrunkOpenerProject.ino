/*************************************************************
Project Title: Smart Trunk Opener : Arduino-based hands-free trunk opener
Description: An Arduino-based hands-free trunk release system designed to improve vehicle accessibility.
Unlike standard proximity sensors that can trigger accidentally, this system utilizes a "continuous wave"
verification logic. The trunk only unlocks if an IR signal is maintained for a consistent 5-second duration,
filtering out noise and accidental triggers from pedestrians or animals.
Features:
1. Continuous detection logic (prevents false positives).
2. Adjustable "Grace Period" for signal flickering.

*************************************************************/
// Basic configuration
// Prevents re-triggering during the same wave
bool hasTriggered = false;
// D2 PIN - Connected to IR Sensor (2 IR coupled together)
const int irPin = 2;
unsigned long motionStartTime = 0;
unsigned long lastSeenTime = 0;
bool isTracking = false;

// Currently 5 second for active motion
const unsigned long triggerDuration = 5000;
const unsigned long gracePeriod = 700;

void setup() {
  pinMode(irPin, INPUT);
  Serial.begin(9600);
}

void loop() {
  bool legMovementPresent = (digitalRead(irPin) == LOW);
  unsigned long currentTime = millis();

  if (legMovementPresent) {
    if (!isTracking) {
      motionStartTime = currentTime;
      isTracking = true;
      // Ready for a new detection cycle
      hasTriggered = false;
    }
    lastSeenTime = currentTime; 
  } else {
    // If hand is gone longer than the grace period, reset everything
    if (currentTime - lastSeenTime > gracePeriod) {
      isTracking = false;
      hasTriggered = false;
    }
  }

  if (isTracking && !hasTriggered) {
    unsigned long duration = currentTime - motionStartTime;
    
    if (duration >= triggerDuration) {
      // Mark as triggered so it doesn't loop
      hasTriggered = true;
      // Reset the start time so the NEXT 5 seconds can start immediately
      motionStartTime = currentTime;
      Serial.println("ACTION TRIGGERED: 5 seconds reached!");
    }
  }
  delay(50);
}
