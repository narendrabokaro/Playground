#include <FS.h>
 
unsigned long lastServerUpdateTime;
String fileName = "/logdata.txt";
int count = 0;

void writeFile(String msg) {
    File file = SPIFFS.open(fileName, "a");

    if (!file) {
      Serial.println("Error opening file for writing");
      return;
    }

    int bytesWritten = file.print(msg);

    if (bytesWritten == 0) {
      Serial.println("File write failed");
      return;
    }
  
    file.close(); 
}

void createFile() {
    File file = SPIFFS.open(fileName, "w");

    if (!file) {
      Serial.println("Error opening file for writing");
      return;
    }

    int bytesWritten = file.print("Date:Time:Location:Status\n");

    if (bytesWritten == 0) {
      Serial.println("File write failed");
      return;
    }
  
    file.close();
}

void readFile() {
    File file = SPIFFS.open(fileName, "r");

    if (!file) {
      Serial.println("Failed to open file for reading");
      return;
    }

    Serial.println("File Content:");

    while (file.available()) {
      Serial.write(file.read());
    }

    file.close();
}

void fileSystemMount() {
    bool success = SPIFFS.begin();

    if (!success) {
      Serial.println("Error mounting the file system");
      return;
    }

    // If file not exists then create it first
    if (!SPIFFS.exists(fileName)) {
        Serial.println("Preparing a fresh file to write.");
        createFile();
    } else {
        Serial.print("Good, file exist .. lets read the file data");
        readFile();
    }
}

void setup() {
 
  Serial.begin(115200);
  Serial.println();
 
  fileSystemMount();
}

void loop() {
    // unsigned long now = millis();

    // // Update uptime after every 10 second 
    // if (now - lastServerUpdateTime >= 10000) {
    //     if (count++ % 2) {
    //         lastServerUpdateTime = now;
    //         writeFile("13/03/2023:13.02.23:Kitchen:ON\n");
    //         Serial.println("File write successful.");
    //     } else {
    //         writeFile("14/03/2023:14.22.26:Kitchen:OFF\n");
    //     }
    // }
}