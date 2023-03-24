#include <FS.h>
 
unsigned long lastServerUpdateTime;
int count = 0;

void setup() {
 
  Serial.begin(115200);
  Serial.println();
 
  bool success = SPIFFS.begin();
 
  if (!success) {
    Serial.println("Error mounting the file system");
    return;
  }
 
  // File file = SPIFFS.open("/file.txt", "w");
 
  // if (!file) {
  //   Serial.println("Error opening file for writing");
  //   return;
  // }
 
  // int bytesWritten = file.print("12/03/2023 13.02:Kitchen bulb switched ON\n");
  // file.print("12/03/2023 13.32:Kitchen bulb switched OFF\n");
  // file.print("12/03/2023 14.02:Outside bulb switched ON\n");
 
  // if (bytesWritten == 0) {
  //   Serial.println("File write failed");
  //   return;
  // }
 
  // file.close(); 
  File file2 = SPIFFS.open("/file.txt", "r");

  if (!file2) {
    Serial.println("Failed to open file for reading");
    return;
  }
 
  Serial.println("File Content:");
 
  while (file2.available()) {
    Serial.write(file2.read());
  }
 
  file2.close();
}

void loop() {
    unsigned long now = millis();

    // Update uptime after every second 
    // if (now - lastServerUpdateTime >= 5000) {
    //     if (count++ < 5) {
    //         lastServerUpdateTime = now;
    //         File file = SPIFFS.open("/file.txt", "a");

    //         if (!file) {
    //           Serial.println("Error opening file for writing");
    //           return;
    //         }

    //         int bytesWritten = file.print("12/03/2023 13.02:Kitchen bulb switched ON\n"); 
    //         if (bytesWritten == 0) {
    //           Serial.println("File write failed");
    //           return;
    //         }
    //         file.close();
    //         Serial.println("File write successful.");
    //     }
    // }
}