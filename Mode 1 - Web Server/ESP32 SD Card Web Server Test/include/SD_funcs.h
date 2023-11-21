#ifndef SD_FUNCS_H
#define SD_FUNCS_H

#include <FS.h>
#include <SD.h>
#include <SPI.h>

extern void SD_init(fs::FS &fs);
extern void SD_printType(fs::FS &fs);
extern void SD_printSize(fs::FS &fs);
extern void SD_printRootDir(fs::FS &fs);
extern void SD_createDir(fs::FS &fs, const char* path);
extern void SD_removeDir(fs::FS &fs, const char* path);
extern void SD_readFile(fs::FS &fs, const char* path);
extern void SD_appendFile(fs::FS &fs, const char* path, const char* message);


#endif // SD_FUNCS_H
