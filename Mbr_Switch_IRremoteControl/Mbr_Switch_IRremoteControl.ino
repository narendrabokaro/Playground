/*************************************************************
  Filename - MBR and GBR switch remote - NodeMCU version
  Description - Designed for special requirement where master switch panel is not easily accessible.
  Requirement -
    > Supports IR remote control operation and normal panel switching operation.

  version - 1.0.0
 *************************************************************/

#include <IRremote.h>

// Totally 5 GIPO using for this program as of now
IRrecv IR(D7);
// D1 and D2 left for RTO
#define fanRelay D3
#define tubeLightRelay D4
#define fanSwitch D5
#define tubeLightSwitch D6

// Tell whether FAN/ Tubeligh are On/ Off
boolean isFanOn = false;
boolean isTubelightOn = false;

void setup() {
  Serial.begin(9600);
  IR.enableIRIn();

  pinMode(fanRelay, OUTPUT);
  pinMode(tubeLightRelay, OUTPUT);

  pinMode(fanSwitch, INPUT_PULLUP);
  pinMode(tubeLightSwitch, INPUT_PULLUP);

  digitalWrite(fanRelay, HIGH);
  digitalWrite(tubeLightRelay, HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (IR.decode()) {
      Serial.println(IR.decodedIRData.decodedRawData, HEX);

      // TODO - Read and write the approriate code for buttons
      // Switch On the FAN When IR remote button 1 is pressed
      if (IR.decodedIRData.decodedRawData == 0xF30CFF00) {
          digitalWrite(fanRelay, LOW);
          Serial.println("Button 1 pressed");
      }

      // Switch Off the FAN When IR remote button 2 is pressed
      if (IR.decodedIRData.decodedRawData == 0xE718FF00) {
          digitalWrite(fanRelay, HIGH);
          Serial.println("Button 2 pressed");
      }

      // Switch On the TubeLight When IR remote button 3 is pressed
      if (IR.decodedIRData.decodedRawData == 0xA15EFF00) {
          digitalWrite(tubeLightRelay, LOW);
          Serial.println("Button 3 pressed");
      }

      // Switch Off the Tubelight When IR remote button 4 is pressed
      if (IR.decodedIRData.decodedRawData == 0xF708FF00) {
          digitalWrite(tubeLightRelay, HIGH);
          Serial.println("Button 4 pressed");
      }

      // delay(1000);
      IR.resume();
  }

  // when FAN switch ON
  if (digitalRead(fanSwitch) == LOW && !isFanOn) {
      // Turn ON the device by making relay LOW
      // Turn OFF the device by making relay HIGH
      digitalWrite(fanRelay, LOW);
      Serial.println("Fan switched pressed ON");

      // Set the flag true
      isFanOn = true;
  }

  // when FAN switch OFF
  if (digitalRead(fanSwitch) == HIGH && isFanOn) {
      digitalWrite(fanRelay, HIGH);
      Serial.println("Fan switched pressed OFF");

      // Set the flag false
      isFanOn = false;
      delay(100);
  }

  // when TubeLight switch ON
  if (digitalRead(tubeLightSwitch) == LOW && !isTubelightOn) {
      digitalWrite(tubeLightRelay, LOW);
      Serial.println("Tubelight switched pressed ON");
      // Set the flag true
      isTubelightOn = true;
  }

  // when TubeLight switch OFF
  if (digitalRead(tubeLightSwitch) == HIGH && isTubelightOn) {
      digitalWrite(tubeLightRelay, HIGH);
      Serial.println("Tubelight switched pressed OFF");
      // Set the flag false
      isTubelightOn = false;
      delay(100);
  }
1000
  delay();
}
