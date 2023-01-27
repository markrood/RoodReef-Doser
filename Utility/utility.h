#ifndef UTILITY_H_
#define UTILITY_H_

#include "Arduino.h"
#include "SPIFFS.h"



class Utility{
    public:
        String readFile(fs::FS &fs, const char * path);
        void writeFile(fs::FS &fs, const char * path, const char * message);
        void startSpiffs();


    private:
        
};
#endif