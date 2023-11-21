#include "SD_funcs.h"

void SD_init(fs::FS &fs) {
    if (!SD.begin()) {
        Serial.println("SD Card mount failed");
        return;
    }

    if (SD.cardType() == CARD_NONE) {
        Serial.println("No SD Card attached");
    }

    Serial.println("SD Card mounted successfully");
}

void SD_printType(fs::FS &fs) {
    Serial.print("SD Card Type: ");
    switch (SD.cardType()) {
        case CARD_MMC:
            Serial.println("MMC");
            break;

        case CARD_SD:
            Serial.println("SDSC");
            break;

        case CARD_SDHC:
            Serial.println("SDHC");
            break;

        default:
            Serial.println("UNKNOWN");
            break;
    }
}

void SD_printSize(fs::FS &fs) {
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
}

void SD_printRootDir(fs::FS &fs) {
    Serial.println("Printing root directory:");

    File root = fs.open("/");
    if(!root) {
        Serial.println("Failed to open root directory");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.print("  DIR : ");
            Serial.println(file.name());
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }

        file = root.openNextFile();
    }
}

void SD_createDir(fs::FS &fs, const char* path) {
    Serial.printf("Creating Directory: %s\n", path);
    if (fs.mkdir(path)) {
        Serial.println("Directory created successfully");
    } else {
        Serial.println("Create Directory failed");
    }
}

void SD_removeDir(fs::FS &fs, const char* path) {
    Serial.printf("Removing Directory: %s\n", path);
    if (fs.rmdir(path)) {
        Serial.println("Directory removed successfully");
    } else {
        Serial.println("Remove Directory failed");
    }
}

void SD_readFile(fs::FS &fs, const char* path) {
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.println("Read from file:");
    while (file.available()) {
        Serial.write(file.read());
    }

    file.close();
}

void SD_appendFile(fs::FS &fs, const char* path, const char* message) {
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for appending");
        return;
    }

    if (file.print(message)) {
        Serial.println("Message appended successfully");
    } else {
        Serial.println("Append failed");
    }
}