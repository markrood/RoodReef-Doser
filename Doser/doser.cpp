#include "doser.h"
#include "utility.h"

Doser::Doser(AsyncWebServer *server, Database *_db){
    db = _db;
    server = _server;
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
    }else{
        spiffsMounted = true;
    }
    util = new Utility();
    //setDbVariables();
    float amtF = db->getFloat("Doser/Blue/Calibration/secBluPerMl");
    Serial.print("secBluMl from db is: ");
    Serial.println(amtF);
    setBluSecPerMl(amtF);
    amtF = db->getFloat("Doser/Green/Calibration/secGrnPerMl");
    Serial.print("secGrnMl from db is: ");
    Serial.println(amtF);
    setGrnSecPerMl(amtF);
    amtF = db->getFloat("Doser/Yellow/Calibration/secYelPerMl");
    Serial.print("secYelMl from db is: ");
    Serial.println(amtF);
    setYelSecPerMl(amtF);
    amtF = db->getFloat("Doser/Purple/Calibration/secPurPerMl");
    Serial.print("secPurMl from db is: ");
    Serial.println(amtF);
    setPurSecPerMl(amtF);
    int amt = db->getInt("Doser/Blue/Dosing/bluMl");
    Serial.print("bluMl from db is: ");
    Serial.println(amt);
    setBluMl(amt);
    amt = db->getInt("Doser/Green/Dosing/grnMl");
     Serial.print("grnMl from db is: ");
    Serial.println(amt);
   setGrnMl(amt);
    amt = db->getInt("Doser/Yellow/Dosing/yelMl");
     Serial.print("yelMl from db is: ");
    Serial.println(amt);
   setYelMl(amt);
    amt = db->getInt("Doser/Purple/Dosing/purMl");
     Serial.print("purMl from db is: ");
    Serial.println(amt);
   setPurMl(amt);
}

int Doser::calibrate(String color, bool start){
    int retVal = 0;
    if(color == "red"){
        if(start){
            int now = millis();
            retVal = now - startTime;
            motor(1,1);
            //return 0;
        }else{
            int now = millis();
            retVal = now - startTime;
            motor(1,0);
             //return stopTime;
        }
    }else if(color == "green"){
        if(start){
            int now = millis();
            retVal = now - startTime;
            motor(2,1);
            //return 0;
        }else{
            int now = millis();
            retVal = now - startTime;
            motor(2,0);
             //return stopTime;
        }
    }else if(color == "yellow"){
        if(start){
            int now = millis();
            retVal = now - startTime;
            motor(3,1);
            //return 0;
        }else{
           int now = millis();
            retVal = now - startTime;
            motor(3,0);
             //return stopTime;
        }
    }else if(color == "purple"){
        if(start){
            int now = millis();
            retVal = now - startTime;
            motor(4,1);
            //return 0;
        }else{
            int now = millis();
            retVal = now - startTime;
            motor(4,0);
             //return stopTime;
        }
    }
    //Serial.print("retVal is: ");
    //Serial.println(retVal);
    return retVal/1000;
}

bool Doser::dose(int color){
    //Serial.println("D");
    bool retVal = true;
    if(firstDoseTime){
        //Serial.println("D+");
        WebSerial.println("D");
        //Serial.println(firstDose);
        doseRun = 0; //TODO
        firstDoseTime = false;
        if(color == 1){
            doseRun = getBluSecPerMl()*getBluMl();
        }else if(color == 2){
            doseRun = getGrnSecPerMl()*getGrnMl();
        }else if(color == 3){
            doseRun = getYelSecPerMl()*getYelMl();
        }else if(color == 4){
            //Serial.println("12.0");
            doseRun = getPurSecPerMl()*getPurMl();
            //Serial.print("Dose run for purple is: ");
            //Serial.println(doseRun);
            
        }
        firstDose = millis();
        motor(color,1);
    }else{
        //Serial.println("E");
        WebSerial.println("E");
        unsigned long now = millis();      //******************TODO this is running
        elapse = now - firstDose;
        //String strElapse = String(elapse);
        //Serial.print("Elapse is: ");
        //Serial.println(strElapse);
        //Serial.print("doseRun in check is: ");
        //Serial.println(doseRun);
        retVal = true;
    }
    if( elapse/1000 >= doseRun){
        //Serial.println("met time");
          motor(color, 0);
        //Serial.print("Dosing ");
        //Serial.print(color);
        //Serial.print(" for this long: ");
        //Serial.println(doseRun);
        firstDoseTime = true;
        retVal = false;
        elapse = 0;
        lastDoseRun = doseRun;
        //delay(100);
/*   //if it runs TOO long shut them all down
    if(elapse > MAX_DOSE_RUN){
        motor(1,0);
        motor(2,0);
        motor(3,0);
        motor(4,0);
        //TODO once have a decent logger, log dosing ran too long
        retVal = false;
        }
 */    
    }
    return retVal;
}


void Doser::motor(int pump, int value) {
  bool pumpRunning = false;
  if (pump == 1) {
    Serial.print("Blue motor value is: ");
    Serial.println(value);
    digitalWrite(BLUEMOTOR, value);
  } else if (pump == 2) {
    Serial.print("Green motor value is: ");
    Serial.println(value);
    digitalWrite(GREENMOTOR, value);
  } else if (pump == 3) {
    Serial.print("Yellow motor value is: ");
    Serial.println(value);
    digitalWrite(YELLOWMOTOR, value);
  } else if (pump == 4) {
    Serial.print("Purpley motor value is: ");
    Serial.println(value);
    digitalWrite(PURPLEMOTOR, value);
  }
}


bool Doser::setDbVariables(){
    bool retVal = true;
    db->putFloat("/Doser/Blue/Calibration/secBluPerMl",0.47);
    secBluPerMl = db->getFloat("/Doser/Blue/Calibration/secBluPerMl");
    String secBluPerMlStr = String(secBluPerMl);
    Serial.print("Blue cal is: ");
    //Serial.println(secBluPerMlStr);
    db->putInt("/Doser/Blue/Dosing/bluMl",50);
    bluMl = db->getInt("/Doser/Blue/Dosing/bluMl");
    String bluMlStr = String(bluMl);
    util->writeFile(SPIFFS, "/secBluPerMl.txt", secBluPerMlStr.c_str());
    util->writeFile(SPIFFS, "/bluMl.txt", bluMlStr.c_str());
    db->putFloat("/Doser/Green/Calibration/secGrnPerMl",0.48);
    secBluPerMlStr.clear();
    bluMlStr.clear();
    secGrnPerMl = db->getFloat("/Doser/Green/Calibration/secGrnPerMl");
    String secGrnPerMlStr = String (secGrnPerMl);
    db->putInt("/Doser/Green/Dosing/grnMl",51);
    grnMl = db->getInt("/Doser/Green/Dosing/grnMl");
    String grnMlStr = String(grnMl);
    util->writeFile(SPIFFS, "/secGrnPerMl.txt", secBluPerMlStr.c_str());
    util->writeFile(SPIFFS, "/grnMl.txt", bluMlStr.c_str());
    db->putFloat("/Doser/Yellow/Calibration/secYelPerMl",0.49);
    secYelPerMl = db->getFloat("/Doser/Yellow/Calibration/secYelPerMl");
    String secYelPerMlStr = String (secYelPerMl);
    db->putInt("/Doser/Yellow/Dosing/yelMl",52);
    yelMl = db->getInt("/Doser/Yellow/Dosing/yelMl");
    String yelMlStr = String(yelMl);
    util->writeFile(SPIFFS, "/secYelPerMl.txt", secYelPerMlStr.c_str());
    util->writeFile(SPIFFS, "/yelMl.txt", yelMlStr.c_str());
    db->putFloat("/Doser/Purple/Calibration/secPurPerMl",0.5);
    secPurPerMl = db->getFloat("/Doser/Purple/Calibration/secPurPerMl");
    String secPurPerMlStr = String (secPurPerMl);
    db->putInt("/Doser/Purple/Dosing/purMl",53);
    purMl = db->getInt("/Doser/Purple/Dosing/purMl");
    String purMlStr = String(purMl);
    util->writeFile(SPIFFS, "/secPurPerMl.txt", secPurPerMlStr.c_str());
    util->writeFile(SPIFFS, "/purMl.txt", purMlStr.c_str());
    return retVal;
}



float Doser::getBluSecPerMl(){
    return secBluPerMl;
}

void Doser::setBluSecPerMl(float secPerMl){
    secBluPerMl = secPerMl;
}

int Doser::getBluMl(){
    return bluMl;
}

void Doser::setBluMl(int blMl){
 bluMl = blMl;
}

float Doser::getGrnSecPerMl(){
    return secGrnPerMl;
}

void Doser::setGrnSecPerMl(float secPerMl){
    secGrnPerMl = secPerMl;
}

int Doser::getGrnMl(){
    return grnMl;
}

void Doser::setGrnMl(int grMl){
    grnMl = grMl;
}

float Doser::getYelSecPerMl(){
    return secYelPerMl;
}

void Doser::setYelSecPerMl(float secPerMl){
    secYelPerMl = secPerMl;
}

int Doser::getYelMl(){
    return yelMl;
}

void Doser::setYelMl(int ywMl){
    yelMl = ywMl;
}

float Doser::getPurSecPerMl(){
    return secPurPerMl;
}

void Doser::setPurSecPerMl(float secPerMl){
    secPurPerMl = secPerMl;
}

int Doser::getPurMl(){
    return purMl;
}

void Doser::setPurMl(int puMl){
    purMl = puMl;
}



int Doser::getErrorCode(){
    return errorCode;
}
void Doser::setErrorCode(int errCode){
    errorCode = errCode;
}

int Doser::getDoseRun(){
    return lastDoseRun;
}
//TODO call this with the automatic db  update
void Doser::updateDoserDb(){


    secBluPerMl = db->getFloat("Doser/Blue/Calibration/secBluPerMl");

    bluMl = db->getInt("Doser/Blue/Dosing/bluMl");

    secGrnPerMl = db->getFloat("Doser/Green/Calibration/secGrnPerMl");

    grnMl = db->getInt("Doser/Green/Dosing/grnMl");
 
    secYelPerMl = db->getFloat("Doser/Yellow/Calibration/secYelPerMl");

    grnMl = db->getInt("Doser/Green/Dosing/grnMl");
 
    yelMl = db->getInt("Doser/Yellow/Dosing/yelMl");

   // util->writeFile(SPIFFS, "/secPurPerMl.txt", secPurPerMlStr.c_str());
    secPurPerMl = db->getFloat("Doser/Purple/Calibration/secPurPerMl");



    //util->writeFile(SPIFFS, "/purMl.txt", purMlStr.c_str());
    purMl = db->getInt("Doser/Purple/Dosing/purMl");
}