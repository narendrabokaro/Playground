/*************************************************************
  Filename - MBR and GBR switch remote - NodeMCU version
  Description - Designed for special requirement where master switch panel is not easily accessible.
  Requirement -
    > Supports IR remote control operation and normal panel switching operation.

  version - 2.0.0
 *************************************************************/
// For RTC module (DS1307)
#include "RTClib.h"
#include <IRremote.h>
#include <EEPROM.h>

// D1 and D2 allotted to RTC module DS1307 (I2C support)
RTC_DS1307 rtc;
DateTime currentTime;
IRrecv IR(D7);

#define fanRelay D3
#define tubeLightRelay D4
#define fanSwitch D5
#define tubeLightSwitch D6

// Tell whether FAN/ Tubeligh are On/ Off
boolean isFanOn = false;
boolean isTubelightOn = false;

// Memory addresses to maintain states of all devices
// ESP8266 takes 4 bytes of memory for allocation
int fanMemAddr = 0;
int tubeMemAddr = 4;

// Time for various comparision
struct Time {
    int hours;
    int minutes;
};

// For RTC module setup
void rtcSetup() {
    Serial.println("rtcSetup :: Health status check");
    delay(1000);

    if (!rtc.begin()) {
        Serial.println("rtcSetup :: Couldn't find RTC");
        Serial.flush();
        while (1) delay(10);
    }

    if (!rtc.isrunning()) {
        Serial.println("rtcSetup :: RTC is NOT running, Please uncomment below lines to set the time!");
        // When time needs to be set on a new device, or after a power loss, the
        // following line sets the RTC to the date & time this sketch was compiled
        //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        
        // This line sets the RTC with an explicit date & time, for example to set
        // January 21, 2014 at 3am you would call:
        // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    }

    Serial.println("rtcSetup :: RTC is running fine and Current time >");
    currentTime = rtc.now();
    Serial.print(currentTime.hour());
    Serial.print(":");
    Serial.print(currentTime.minute());
    Serial.print(":");
    Serial.print(currentTime.second());
}

void setup() {
    // ESP8266 have 512 bytes of internal EEPROM
    EEPROM.begin(512);
    Serial.begin(9600);

    IR.enableIRIn();
    // Setup the RTC mmodule
    rtcSetup();

    pinMode(fanRelay, OUTPUT);
    pinMode(tubeLightRelay, OUTPUT);

    pinMode(fanSwitch, INPUT_PULLUP);
    pinMode(tubeLightSwitch, INPUT_PULLUP);

    // By Default, all devices switched off 
    digitalWrite(fanRelay, HIGH);
    digitalWrite(tubeLightRelay, HIGH);

    // Turn On devices in case of power restoration 
    actionBasedOnDeviceState();
}

int readMemory(int addr) {
    // Read the memory address
    return EEPROM.read(addr);
}

void writeMemory(int addr, int writeValue) {
    // Write the memory address
    EEPROM.write(addr, writeValue);
    EEPROM.commit();
}

void actionBasedOnDeviceState() {
    // Check for the fan last state
    if (readMemory(fanMemAddr) == 1) {
        turnDevice(fanRelay, 1);
    }

    if (readMemory(tubeMemAddr) == 1) {
        turnDevice(tubeLightRelay, 1);
    }
}

// Int turndeviceON [0 to switch OFF the device | 1 to switch ON the device]
void turnDevice(int deviceRelayName, int turndeviceON) {
    // Turn ON/ OFF the devices
    digitalWrite(deviceRelayName, turndeviceON ? LOW : HIGH);

    // Now read/ write from memory
    writeMemory(deviceRelayName == D3 ? fanMemAddr : tubeMemAddr, turndeviceON);
}

void loop() {
    // Handles all Infrared remote operations
    if (IR.decode()) {
        Serial.println(IR.decodedIRData.decodedRawData, HEX);

        // TODO - Read and write the approriate code for buttons
        // Switch On the FAN When IR remote button 1 is pressed
        if (IR.decodedIRData.decodedRawData == 0xF30CFF00) {
            // digitalWrite(fanRelay, LOW);
            turnDevice(fanRelay, 1);
            Serial.println("Button 1 pressed");
        }

        // Switch Off the FAN When IR remote button 2 is pressed
        if (IR.decodedIRData.decodedRawData == 0xE718FF00) {
            // digitalWrite(fanRelay, HIGH);
            turnDevice(fanRelay, 0);
            Serial.println("Button 2 pressed");
        }

        // Switch On the TubeLight When IR remote button 3 is pressed
        if (IR.decodedIRData.decodedRawData == 0xA15EFF00) {
            // digitalWrite(tubeLightRelay, LOW);
            turnDevice(tubeLightRelay, 1);
            Serial.println("Button 3 pressed");
        }

        // Switch Off the Tubelight When IR remote button 4 is pressed
        if (IR.decodedIRData.decodedRawData == 0xF708FF00) {
            // digitalWrite(tubeLightRelay, HIGH);
            turnDevice(tubeLightRelay, 0);
            Serial.println("Button 4 pressed");
        }

        IR.resume();
  }

  // Handles all wall switch operations
  if (digitalRead(fanSwitch) == LOW && !isFanOn) {
      // Turn ON the device by making relay LOW
      // Turn OFF the device by making relay HIGH
      // digitalWrite(fanRelay, LOW);
      turnDevice(fanRelay, 1);
      Serial.println("Fan switched pressed ON");

      // Set the flag true
      isFanOn = true;
  }

  // when FAN switch OFF
  if (digitalRead(fanSwitch) == HIGH && isFanOn) {
      // digitalWrite(fanRelay, HIGH);
      turnDevice(fanRelay, 0);
      Serial.println("Fan switched pressed OFF");

      // Set the flag false
      isFanOn = false;
      delay(100);
  }

  // when TubeLight switch ON
  if (digitalRead(tubeLightSwitch) == LOW && !isTubelightOn) {
      // digitalWrite(tubeLightRelay, LOW);
      turnDevice(tubeLightRelay, 1);
      Serial.println("Tubelight switched pressed ON");
      // Set the flag true
      isTubelightOn = true;
  }

  // when TubeLight switch OFF
  if (digitalRead(tubeLightSwitch) == HIGH && isTubelightOn) {
      // digitalWrite(tubeLightRelay, HIGH);
      turnDevice(tubeLightRelay, 0);
      Serial.println("Tubelight switched pressed OFF");
      // Set the flag false
      isTubelightOn = false;
      delay(100);
  }

  delay(500);
}
