// Pin Configuration
const int irSensorPin = 2;

void setup() {
  pinMode(irSensorPin, INPUT);
  Serial.begin(9600);
  Serial.println("IR Sensor Ready...");
}

void loop() {
  // Read the sensor value (LOW usually means object detected)
  int sensorValue = digitalRead(irSensorPin);

  if (sensorValue == LOW) {
    // Object Detected
    Serial.println("Object Detected!"); 
  } else {
    // Path Clear
  }

  // Small delay for stability
  delay(100);
}
