#ifndef DOSER_H_
#define DOSER_H_

#include <Arduino.h>
#include "fbdb.h"
#include "utility.h"
#include <WebSerial.h>

class Doser{
    public:
        Doser(AsyncWebServer *server, Database *_db); 

        int calibrate(String color, bool start);
        bool dose(int color);
        void motor(int pump, int value);
        float getBluSecPerMl();
        void setBluSecPerMl(float secPerMl);
        int getBluMl();
        void setBluMl(int blMl);
        float getGrnSecPerMl();
        void setGrnSecPerMl(float secPerMl);
        int getGrnMl();
        void setGrnMl(int grMl);
        float getYelSecPerMl();
        void setYelSecPerMl(float secPerMl);
        int getYelMl();
        void setYelMl(int ywMl);
        float getPurSecPerMl();
        void setPurSecPerMl(float secPerMl);
        int getPurMl();
        void setPurMl(int puMl);
        int getErrorCode();
        void setErrorCode(int errCode);
        bool setDbVariables();
        int getDoseRun();
        void updateDoserDb();

    private:
        Database *db;
        Utility *util;
        AsyncWebServer *_server;
        bool spiffsMounted = false;
        float secBluPerMl = 1/2.01;
        int bluMl = 50;
        float secGrnPerMl = 1/2.01;
        int grnMl = 50;
        float secYelPerMl = 1/2.01;
        int yelMl = 50;
        float secPurPerMl = 1/2.01;
        int purMl = 50;
        float doseRun = 0;
        int doseTime = 0;
        unsigned long firstDose = 0;
        bool firstDoseTime = true;
        unsigned long elapse = 0;
        int MAX_DOSE_RUN = 80000;
        int startTime = 0;
        int stopTime = 0;
        int BLUEMOTOR = 33;  
        int GREENMOTOR = 26;  
        int YELLOWMOTOR = 36; //12
        int PURPLEMOTOR = 39; //13
        long doseStartTime = 0;
        int errorCode = 0;
        int lastDoseRun = 0;

        tm timeinfo;
        time_t now;
};

#endif