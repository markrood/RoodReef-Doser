/***********************************************begin************************/
//  Description:{}
//    This main class just loops and calls tick and tock to run the
//    event machine in FishScheduler.  The if loops thru all 31 events
//    (i.e. hour,15min,30min,2h,3h,4h,8h,12h,1am-noon, 1pm-midnight) to see
//    if that event is set.  If one of these events is set It will do what 
//    is scheduled to run on this event and then reset the event.  In this 
//    case it will dose blue, green, yellow, or purple on a particular event.
//
//    This main class also runs the Dose calibration.  It will read a button
//    pressed to start the calibration and read the subsequent button push
//    to stop the calibration.  Dose class has the calibration funtion...
//    so this main in call dose->calibrate.
//
// Classes: 
//    Doser
//
//      Methods
//        dose(String color)
//        calibrate(String color,bool start)
//
//  Scheduler
//    Methods
//      updateMyTime()
//      initTime()
//      tick()   --does hour
//      tock()   --does minute
//      isFlagSet()  -- did a cron go off and which one (1-31)
//      resetFlag()  -- turns cron indicator off
//      
// ERROR CODES
//  M_wfnc   -- wifi is not connected
//  D_B_tl   -- Dosed Blue ran too long...shut dosing down
//  D_G_tl   -- Dosed Green ran too long...shut dosing down
//  D_Y_tl   -- Dosed Yellow ran too long...shut dosing down
//  D_P_tl   -- Dosed Purple ran too long...shut dosing down 
//  M_ftgt   -- failed to get time from internet
//
//  Pins
//    BLUEMOTOR = 5;
//    GREENMOTOR = 33;
//    YELLOWMOTOR = 32;
//    PURPLEMOTOR = 15;
//    BLU_BTN_PIN = 26;  TODO get real values for cal buttons
//    GRN_BTN_PIN =27;
//    YW_BTN_PIN = 14;
///   PU_BTN_PIN = 12;
//
/***********************************************************************/


#include <Arduino.h>
#include "AsyncTCP.h"

#include "ESPAsyncWebServer.h"
#include "ESPAsyncWiFiManager.h"         //https://gitWiFiManagerhub.com/tzapu/WiFiManager
//#include <WiFiManager.h>
#include <WebSerial.h>
#include <ESPmDNS.h>
#include "fishScheduler.h"
#include "button.h"
#include "doser.h"
#include "fbdb.h"
#include "utility.h"
//#include "SPIFFS.h"
#include "Effortless_SPIFFS.h"
#include "ArduinoJson.h"
//#include <Wire.h>  // must be included here so that Arduino library object file references work
//#include <RtcDS3231.h>

//RtcDS3231<TwoWire> Rtc(Wire);

AsyncWebServer server(80);
DNSServer dns;
void configModeCallback (AsyncWiFiManager *myWiFiManager);
int addDailyDoseAmt(int color,int val);
bool writeDailyDoseAmtToDb(String color, int amt);
//void startSpiffs();
//String readFile(fs::FS &fs, const char * path);
//void writeFile(fs::FS &fs, const char * path, const char * message);
bool writeDailyDosesToDb();
bool writeCalibrationToDb(int color, float amt);
void setDate();
int sendHttp(String event);
/*void setTheTime(tm localTime);
bool getNTPtime(int sec);
//bool isDateTimeValid();
//void printDateTime(RtcDateTime now);
void setRtcTime();
int getCurrentTime(String input); */

void doseBlue(int evt);
void doseGreen(int evt);
void doseYellow(int evt);
void dosePurple(int evt);

bool notDosing();

void checkDosingSched(int i);


int led = 2;
FishSched *mySched;
Doser *doser;
Database *db;
static Utility *util;
//FishTime *myTime;
    enum{Hour,Fifteen,Thirty,TwoHour,ThreeHour,FourHour,EightHour,TwelveHour,
        Midnight,OneAm,TwoAm,ThreeAm,FourAm,FiveAm,SixAm,SevenAm,EightAm,NineAm,TenAm,ElevenAm,
        Noon,OnePm,TwoPm,ThreePm,FourPm,FivePm,SixPm,SevenPm,EightPm,NinePm,TenPm,ElevenPm,
        Day,EveryOtherDay,Week,EveryOtherWeek,Month,NumFlags
        };

//calibration buttons
Button blueBtn;
Button greenBtn;
Button yellowBtn;
Button purpleBtn;
//TODO set these to the correct hardware pins for calibration pins/buttons
int BLU_BTN_PIN = 19;
int GRN_BTN_PIN = 18;
int YW_BTN_PIN = 23;
int PU_BTN_PIN = 17;
int BLUEMOTOR = 33;
int GREENMOTOR = 26;
int YELLOWMOTOR = 36; //12
int PURPLEMOTOR = 39; //13
int LED = 2;

int MINCALTIME = 10;
int count = 0;

bool calibrationRunning = false;
bool blueDosing = false;
bool greenDosing = false;
bool yellowDosing = false;
bool purpleDosing = false;
bool bluCalRunning = false;
bool grnCalRunning = false;
bool ywCalRunning = false;
bool puCalRunning= false;

//TODO button test junk remove
long bluBtnTime = 0;
long bluBtnMax = 49000;
bool bluBtnFirst = true;
bool bluePushed = false;
bool greenPushed = false;
bool yellowPushed = false;
bool purplePushed = false;
bool printedStart = false;
bool printedStop = false;
bool blueWasPressed = false;
bool greenWasPressed = false;
bool yellowWasPressed = false;
bool purpleWasPressed = false;
int buttonClrCnt = 0;

int blueDailyDoseAmt = 0;
int greenDailyDoseAmt = 0;
int yellowDailyDoseAmt = 0;
int purpleDailyDoseAmt = 0;
String blueDailyDoseAmtStr = "";
String greenDailyDoseAmtStr = "";
String yellowDailyDoseAmtStr = "";
String purpleDailyDoseAmtStr = "";
  int x = 0;
  int w = 0;
  bool blinking = false;

eSPIFFS fileSystem;

 int yr = 0;
  String yrStr = "";
  int mo = 0;
  int da = 0;
  String daStr = "";
  bool restart = false;

const char* host = "maker.ifttt.com";  //used in sendHttp
const int httpsPort = 80;  //used in sendHttp
String url = "";  //used in sendHttp
WiFiClient client;  //this is passed into Dosing constructor for connecting to doser
String iPAddress;

int newDay = 0;
//tm timeinfo;
//time_t now;

/*int monthDay;
int currentMonth;
String currentMonthName;
int currentYear;
String formattedTime;
int currentDay;
int currentHour;
int currentMinute;
int currentSecond; */

bool wifiConnected = true; //TODO do this right

int** evtPumpArr;

bool bluePumpPending = false;
bool greenPumpPending = false;
bool yellowPumpPending = false;
bool purplePumpPending = false;
bool blueDosed = false;
bool greenDosed = false;
bool yellowDosed = false;
bool purpleDosed = false;
bool alreadyReset = false;
unsigned long sendDataPrevMillis = 0;

bool midnightDone = false;
bool thisIsAPumpEvent = false;
bool aFlagWasSetInLoop = false;

//bool Database::dataChanged = false;

//TODO 
//int testInt = 0; //just a test int for faking day REMOVE it


void setup() {
  // put your setup code here, to run once:
  //Serial.begin(9600);
  Serial.begin(115200);
  //pinMode(led, OUTPUT);
  pinMode(BLU_BTN_PIN,INPUT_PULLUP);
  pinMode(GRN_BTN_PIN,INPUT_PULLUP);
  pinMode(YW_BTN_PIN,INPUT_PULLUP);
  pinMode(PU_BTN_PIN,INPUT_PULLUP);

  pinMode(BLUEMOTOR,OUTPUT);
  pinMode(GREENMOTOR,OUTPUT);
  pinMode(YELLOWMOTOR,OUTPUT);
  pinMode(PURPLEMOTOR,OUTPUT);
  pinMode(LED,OUTPUT);

    ///////////////////Start WiFi ///////////////////////////////////////
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  AsyncWiFiManager wifiManager(&server, &dns);
  //reset settings - for testing
  //wifiManager.resetSettings();
  //wifiManager.setSTAStaticIPConfig(IPAddress(192,168,1,175), IPAddress(192,168,1,1), IPAddress(255,255,255,0), IPAddress(192,168,1,1), IPAddress(192,168,1,1));
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("Doser")) {
    Serial.println("failed to connect and hit timeout");
    Serial.println("restarting esp");
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(1000);
  }
  delay(50);
  //Serial.print("FreeHeap is :");
  //Serial.println(ESP.getFreeHeap());
  delay(50);
  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  Serial.println("local ip");
  Serial.println(WiFi.localIP());

  server.begin();
 WebSerial.begin(&server);
 delay(100);
  //WebSerial.print("local ip is: ");
  //WebSerial.println(WiFi.localIP());
  mySched = new FishSched();
  db  = new Database();
  db->initDb();
  doser = new Doser(&server,db);
  //util = new Utility;
   
  // mySched->initTime();
   mySched->updateMyTime();
  //setup calibrate buttons
  blueBtn.begin(BLU_BTN_PIN);
  greenBtn.begin(GRN_BTN_PIN);
  yellowBtn.begin(YW_BTN_PIN);
  purpleBtn.begin(PU_BTN_PIN);
  //int da = 15;

   // Create a eSPIFFS class
  #ifndef USE_SERIAL_DEBUG_FOR_eSPIFFS
    // Create fileSystem
    

    // Check Flash Size - Always try to incorrperate a check when not debugging to know if you have set the SPIFFS correctly
    if (!fileSystem.checkFlashConfig()) {
      Serial.println("Flash size was not correct! Please check your SPIFFS config and try again");
      delay(100000);
      ESP.restart();
    }
  #else
    // Create fileSystem with debug output
    eSPIFFS fileSystem(&Serial);  // Optional - allow the methods to print debug
  #endif
  
  // Serial.begin(115200);
  //evtPumpArr = db->getEvtPump();
  //util->startSpiffs();
  String test = "test";
  //util->writeFile(SPIFFS,"/test", test.c_str());
  //fileSystem.saveToFile("restart.txt" 0);
  //db->initDb();
  setDate();
  db->setEvents(0);
   delay(10);
}

void loop() {
//TODO remove just fashing led......
//Serial.println("0");
//mySched->printArray();
  if(blinking){
    digitalWrite(LED, 0);
    blinking = false;
  }else{
    digitalWrite(LED, 1);
    blinking = true;
  }
  //check if any calibraton button is pressed and only do that
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    count++;
    //db->callBack();
  }

  if(Database::dataChanged){
    db->setEvents(0);
    Database::dataChanged = false;
  }
  //Serial.println("0.0.0");
  bool bluePushed = blueBtn.debounce();
  //Serial.println("0.0.1");
  bool greenPushed = greenBtn.debounce();
  //Serial.println("0.0.2");
  bool yellowPushed = yellowBtn.debounce();
  //Serial.println("0.0.3");
  bool purplePushed = purpleBtn.debounce();
  //Serial.println("0.0.4");

   while (WiFi.status() != WL_CONNECTED ){
    //TODO db->callBack should be called
    if(w < 2){
      Serial.print('!');
      WebSerial.print("M_wfnc");
      //Serial.println("-1");
      delay(1000);
      w = 0;
      break;
    }else{
      w++;
    }
  }

  
  //Serial.println(WiFi.localIP());

  if(bluePushed && !grnCalRunning && !ywCalRunning && !puCalRunning){
    bluePushed = false;
//Serial.println("0.0");
    if(!bluCalRunning){
      bluCalRunning = true;
      calibrationRunning = true;
    }else{
      bluCalRunning = false;
    }
  }

  if(greenPushed && !bluCalRunning && !ywCalRunning && !puCalRunning){
    //Serial.println("?????????????HERE");
//Serial.println("0.001");
    greenPushed = false;
//Serial.println("0.1");
    if(!grnCalRunning){
      grnCalRunning = true;
      calibrationRunning = true;
    }else{
      grnCalRunning = false;
    }
  }

  
  if(yellowPushed && !grnCalRunning && !bluCalRunning && !puCalRunning){
    yellowPushed = false;
//Serial.println("0.3");
    if(!ywCalRunning){
      ywCalRunning = true;
      calibrationRunning = true;
    }else{
      ywCalRunning = false;
    }
  }
  
  if(purplePushed && !grnCalRunning && !ywCalRunning && !bluCalRunning){
    purplePushed = false;
//Serial.println("0.4");
    if(!puCalRunning){
      puCalRunning = true;
      calibrationRunning = true;
    }else{
      puCalRunning = false;
    }
  }
  
//TODO remove this button test
/*    if(bluBtnFirst){
      bluBtnTime = millis();
      bluBtnFirst = false;
      bluePushed = false;
    }
    long rightNow = millis();
    if(rightNow - bluBtnTime > bluBtnMax){
      bluBtnFirst = true;
      bluePushed = true;
      blueWasPressed = true;
      Serial.println("Blue Button Pressed");
    }*/
////////////////////////////////////////////////
///////////////////////////////////STOPPED HERE GREEN BUTTON NOTWORKING
 /*    if(bluBtnFirst){
      bluBtnTime = millis();
      bluBtnFirst = false;
      greenPushed = false;
     }
    long rightNow = millis();
    if(rightNow - bluBtnTime > bluBtnMax){
      bluBtnFirst = true;
      greenPushed = true;
      greenWasPressed = true;
      Serial.println("Green Button Pressed");
    }*/
////////////////////////////////////////////////
/*    if(bluBtnFirst){
      bluBtnTime = millis();
      bluBtnFirst = false;
      yellowPushed = false;
    }
    long rightNow = millis();
    if(rightNow - bluBtnTime > bluBtnMax){
      bluBtnFirst = true;
      yellowPushed = true;
      yellowWasPressed = true;
      Serial.println("Yellow Button Pressed");
    }*/
////////////////////////////////////////////////

/*    if(bluBtnFirst){
      bluBtnTime = millis();
      bluBtnFirst = false;
      purplePushed = false;
    }
    long rightNow = millis();
    if(rightNow - bluBtnTime > bluBtnMax){
      bluBtnFirst = true;
      purplePushed = true;
      purpleWasPressed = true;
      Serial.println("Purple Button Pressed");
      /*buttonClrCnt++;
      if(buttonClrCnt == 2){
        purplePushed = false;
        Serial.println("Yellow pressed");
        yellowPushed = true;
      }
      if(buttonClrCnt == 3){
        purplePushed = true;
        yellowPushed = false;
        buttonClrCnt = 0;
      }
    }*/
////////////////////////////////////////////////
 
  if(calibrationRunning){
    //Serial.println("1");
//Serial.println("0.5");
    if(blueWasPressed){
      if(bluCalRunning && !grnCalRunning && !ywCalRunning && !puCalRunning){
        //start blue cal
        if(!printedStart){
          //Serial.println("Blue cal started");
          Serial.println("Blue cal started");
          doser->calibrate("blue", true);
          printedStart = true;
        }
      }else if(!grnCalRunning && !ywCalRunning && !puCalRunning && calibrationRunning){
        calibrationRunning = false;
        bluCalRunning = false;
        //stop blue cal
        //Serial.println("Blue cal stopped");
        Serial.println("Blue cal stopped");
        int time = doser->calibrate("blue", false);
        if(time > MINCALTIME){
          doser->setBluSecPerMl(time/100);
          bool inserted = writeCalibrationToDb(1, time/100);
        }
        Serial.println(time);
        printedStart = false;
        blueWasPressed = false;
      }
    }
    if(greenWasPressed){
      if(grnCalRunning && !bluCalRunning && !ywCalRunning && !puCalRunning){
        //start blue cal
        //Serial.println("****************i got here*************");
        if(!printedStart){
          //Serial.println("Green cal started");
          Serial.println("Green cal started");
          doser->calibrate("green", true);
          printedStart = true;
        }
      }else if (!bluCalRunning && !ywCalRunning && !puCalRunning && calibrationRunning){
        calibrationRunning = false;
        grnCalRunning = false;
        //stop blue cal
        //Serial.println("Green cal stopped");
        Serial.println("Green cal stopped");
        int time = doser->calibrate("green", false);
        if(time > MINCALTIME){
          doser->setGrnSecPerMl(time/100);
          bool inserted = writeCalibrationToDb(2, time/100);
        }
        Serial.println(time);
        printedStart = false;
        greenWasPressed = false;
      }
    }
    if(yellowWasPressed){
      if(ywCalRunning && !bluCalRunning && !grnCalRunning && !puCalRunning){
        //start blue cal
        if(!printedStart){
          //Serial.println("Yellow cal started");
          Serial.println("Yellow cal started");
          doser->calibrate("yellow", true);
          printedStart = true;
        }
      }else if(!bluCalRunning && !grnCalRunning && !puCalRunning && calibrationRunning){
        calibrationRunning = false;
        ywCalRunning = false;
        //stop blue cal
        //Serial.println("Yellow cal stopped");
        Serial.println("Yellow cal stopped");
        int time = doser->calibrate("yellow", false);
        if(time > MINCALTIME){
          doser->setYelSecPerMl(time/100);
        bool inserted = writeCalibrationToDb(3, time/100);
        }
      Serial.println(time);
        //TODO save mlPerSec to SPIFFS!!!!!!!!!!!!!!
        //Serial.print("purple mlPerSec is: ");
        //Serial.println(mlPerSec);
        printedStart = false;
        yellowWasPressed = false;
      }
    }
    if(purpleWasPressed){
      if(puCalRunning && !grnCalRunning && !ywCalRunning && !bluCalRunning){
        //start blue cal
        if(!printedStart){
          //Serial.println("Purple cal started");
          Serial.println("Purple cal started");
          doser->calibrate("purple", true);
          printedStart = true;
        }
      }else if(!grnCalRunning && !ywCalRunning && !bluCalRunning && calibrationRunning){
        calibrationRunning = false;
        puCalRunning = false;
        //stop blue cal
        //Serial.println("Purple cal stopped");
        Serial.println("Purple cal stopped");
        int time = doser->calibrate("purple", false);
        if(time > MINCALTIME){
          doser->setPurSecPerMl(time/100);
          bool inserted = writeCalibrationToDb(4, time/100);
        }
        //TODO save mlPerSec to SPIFFS!!!!!!!!!!!!!!
        //Serial.print("purple mlPerSec is: ");
        //Serial.println(mlPerSec);
        printedStart = false;
        purpleWasPressed = false;
      }
    }
  }else{
    //Serial.println("2");
   // mySched->printArray();
//Serial.println("1.0");
    //Serial.println("in main about to setNowTime");
    mySched->setNowTime(); //initializes time MUST DO THIS
  // mySched->updateMyTime();
  //mySched->printArray();
    mySched->tick(); //sets hour
//mySched->printArray();
    //Serial.print("Hour is: ");
    //Serial.println(nowHour);
//mySched->printArray();
    mySched->tock();//sets minute
//mySched->printArray();
    //Serial.print("Minute is: ");
    //Serial.println(nowMinute);
  
    //std::bitset<25> flags = mySched->getFlags();

    //TODO remove this /////////////////
  // int randNumber = random(31);
  // mySched->resetFlag(randNumber);
  /////////////////////////////////////

//Serial.println("2.0");
//mySched->printArray();
//Serial.println("Just getting for flag for loop");
  int z = 0;
  for(int i= 0;i<37;i++){
      int flagSet = mySched->isFlagSet(i);
      z++;
      if(flagSet == 1){
        aFlagWasSetInLoop = true;
        //Serial.print("event is: ");
        //Serial.println(i);
        if(i != 1){
          if(i !=2){
            WebSerial.print("Event ");
            WebSerial.print(i);
            WebSerial.print(" just fired");
          }else{
            WebSerial.print("-");
          }
        }else{
          WebSerial.print("+");
        }
        //Serial.println("2,1");
        //Serial.print("event in loop is: ");
        //Serial.println(i);
 ////////////////////////////////////////////////////////////////////////////////////////////////////////
  //one_hour=0, fifteen=1, thirty=2, two_hour=3, three_hour=4, 
  //four_hour=5,eight_hour=6, twelve_hour=7,
  //midnight=8, one_am=9, two_am=10, three_am=11, four_am=12, 
  //five_am=13, six_am=14, severn_am=15,
  //eight_amm=16, nine_am=17 ten_am=18, eleven_am=19,
  // noon=20, one_pm=21, two_pm=22, three_pm=23, 
  //four_pm=24, five_pm=25, six_pm=26, severn_pm=27,
  //eight_pmm=28, nine_pm=29 ten_pm=30, eleven_pm=31,
  //////////////////////////////////////////////////////////////////////////////////////////////////////
  mySched->printArray();
  checkDosingSched(i);
  mySched->printArray();
   /////////////////  one_hour  /////////////////////////////////
/*   if(i== 0 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 0 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 0 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 0 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 0 && notDosing() && !alreadyReset){
      Serial.println("0,0");
      mySched->setFlag(i,0);
    }
  /////////////////  fifteen  /////////////////////////////////
    if(i == 1 && db->isThisEventPumpSet(i, 0)){
      //Serial.println("7");
      doseBlue(i);
    }
    if (i == 1 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 1 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if(i == 1 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i == 1 && notDosing() && !alreadyReset){
      //Serial.println("8");
      Serial.println("0.1");
      mySched->setFlag(i,0);
    }

  /////////////////  thirty  /////////////////////////////////
    if(i== 2 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 2 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 2 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 2 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 2 && notDosing() && !alreadyReset){
      Serial.println("0,2");
      mySched->setFlag(i,0);
    }
   /////////////////  two_hour  /////////////////////////////////
    if(i== 3 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 3 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 3 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 3 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 3 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
   /////////////////  three_hour  /////////////////////////////////
    if(i== 4 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 4 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 4 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 4 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 4 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
  /////////////////  four_hour  /////////////////////////////////
    if(i== 5 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 5 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 5 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 5 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 5 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
  /////////////////  eight_hour  /////////////////////////////////
    if(i== 6 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 6 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 6 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 6 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
     if(i== 6 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
  /////////////////  twelve_hour  /////////////////////////////////
    if(i== 7 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 7 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 7 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 7 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 7 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
  /////////////////  midnight  /////////////////////////////////
    if(i== 8 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 8 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 8 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 8 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 8 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
   /////////////////  one_am  /////////////////////////////////
    if(i== 9 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 9 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 9 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 9 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
     if(i== 9 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
  /////////////////  two_am  /////////////////////////////////
    if(i== 10 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 10 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 10 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 10 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 10 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
   /////////////////  three_am  /////////////////////////////////
    if(i== 11 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 11 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 11 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 11 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 11 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
   /////////////////  four_am  /////////////////////////////////
    if(i== 12 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 12 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 12 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 12 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 12 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
   /////////////////  five_am  /////////////////////////////////
    if(i== 13 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 13 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 13 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 13 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 13 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
   /////////////////  siz_am  /////////////////////////////////
    if(i== 14 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 14 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 14 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 14 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 14 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
   /////////////////  seven_am  /////////////////////////////////
    if(i== 15 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 15 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 15 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 15 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 15 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
   /////////////////  eight_am  /////////////////////////////////
    if(i== 16 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 16 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 16 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 16 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 16 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
   /////////////////  nine_am  /////////////////////////////////
    if(i== 17 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 17 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 17 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 17 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 17 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
   /////////////////  ten_am  /////////////////////////////////
    if(i== 18 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 18 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 18 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 18 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 18 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
   /////////////////  eleven_am  /////////////////////////////////
    if(i== 19 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 19 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 19 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 19 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 19 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
   /////////////////  noon  /////////////////////////////////
    if(i== 20 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 20 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 20 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 20 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 20 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
   /////////////////  one_pm  /////////////////////////////////
    if(i== 21 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 21 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 21 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 21 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 21 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
   /////////////////  two_pm  /////////////////////////////////
    if(i== 22 && db->isThisEventPumpSet(i, 0)){
      Serial.println("2");
      doseBlue(i);
    }
    if (i == 22 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 22 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 22 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 22 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
   /////////////////  three_pm  /////////////////////////////////
    if(i== 23 && db->isThisEventPumpSet(i, 0)){
      Serial.println("3");
      doseBlue(i);
    }
    if (i == 23 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 23 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 23 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 23 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
   /////////////////  four_pm  /////////////////////////////////
    if(i== 24 && db->isThisEventPumpSet(i, 0)){
      Serial.println("4");
      doseBlue(i);
    }
    if (i == 24 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 24 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 24 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 24 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
   /////////////////  five_pm  /////////////////////////////////
    if(i== 25 && db->isThisEventPumpSet(i, 0)){
      Serial.println("5");
      doseBlue(i);
    }
    if (i == 25 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 25 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 25 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 25 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
   /////////////////  siz_pm  /////////////////////////////////
    if(i== 26 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 26 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 26 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 26 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 26 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
   /////////////////  seven_pm  /////////////////////////////////
    if(i== 27 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 27 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 27 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 27 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 27 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
   /////////////////  eight_pm  /////////////////////////////////
    if(i== 28 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 28 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 28 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 28 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }  
     if(i== 28 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
  /////////////////  nine_pm  /////////////////////////////////
    if(i== 29 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 29 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 29 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 29 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
     if(i== 29 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
  /////////////////  ten_pm  /////////////////////////////////
    if(i== 30 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 30 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 30 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 30 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 30 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }
   /////////////////  eleven_pm  /////////////////////////////////
    if(i== 31 && db->isThisEventPumpSet(i, 0)){
      doseBlue(i);
    }
    if (i == 31 && db->isThisEventPumpSet(i, 1)){
      doseGreen(i);
    }
    if(i == 31 && db->isThisEventPumpSet(i, 2)){
      dosePurple(i);
    }
    if (i == 31 && db->isThisEventPumpSet(i, 3)){
      doseYellow(i);
    }
    if(i== 31 && notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }*/
       }//if 
      //Serial.println("out of flagset loop");
  }//for loop
		  if(aFlagWasSetInLoop) {
			  aFlagWasSetInLoop = false;
		  }else {
			  blueDosed = false;
		      greenDosed = false;
		      yellowDosed = false;
		      purpleDosed = false;
		      alreadyReset = false;
		      midnightDone = false;
		  }

    //Serial.println("Dam");
    delay(1000);
    //Serial.println(ESP.getFreeHeap(i));
    //WebSerial.println(ESP.getFreeHeap());
    
    //Serial.print(".");
    WebSerial.print(".");
   
     }//else no cal
}//loop

//////////////////////////////////////////////////////////////
// call back needed for wifi
/////////////////////////////////////////////////////////////
void configModeCallback (AsyncWiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  //myWiFiManager->startConfigPortal("ATOAWC");addDailyDoseAmt
  //myWiFiManager->autoConnect("DOSER");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());

}

int addDailyDoseAmt(int color,int val){
  int retVal =0;
//Serial.println("2.2");
  if(color == 1){
    //Serial.println("2.2.1");
    //blueDailyDoseAmtStr = readFile(SPIFFS, "/blueDailyDoseAmt.txt");
    fileSystem.openFromFile("/blueDailyDoseAmt.txt", blueDailyDoseAmt);
    //Serial.println("2.2.2");
    /*if(blueDailyDoseAmtStr != ""){
      blueDailyDoseAmt = blueDailyDoseAmtStr.toInt();
    }else{
      //TODO
      blueDailyDoseAmt = 0;
    }
    //Serial.println("2.2.3");*/
    blueDailyDoseAmt = blueDailyDoseAmt+val;
    //Serial.println("2.2.4");
    retVal = blueDailyDoseAmt;
    //Serial.println("2.2.5");
    //String blueDailyDoseAmtStrr = String(blueDailyDoseAmt);
    //Serial.println("2.2.6");
    delay(100);
    //util->writeFile(SPIFFS,"/blueDailyDoseAmt.txt", blueDailyDoseAmtStrr.c_str());
    blueDailyDoseAmtStr = String(blueDailyDoseAmt);
    fileSystem.saveToFile("/blueDailyDoseAmt.txt", blueDailyDoseAmtStr.c_str());
     //Serial.println("2.2.7");
   Serial.print("Blue Daily Dose Amt is: ");
    Serial.println(blueDailyDoseAmtStr);
    //blueDailyDoseAmtStr.clear();
    //blueDailyDoseAmtStrr.clear();
    delay(500);
  }else if(color == 2){
     //Serial.println("2.3.1");
    //greenDailyDoseAmtStr = readFile(SPIFFS, "/greenDailyDoseAmt.txt");
    fileSystem.openFromFile("/greenDailyDoseAmt.txt", greenDailyDoseAmt);
     //Serial.println("2.3.2");
     /*if(greenDailyDoseAmtStr != ""){
      greenDailyDoseAmt = greenDailyDoseAmtStr.toInt();
     }else{
      //TODO
      greenDailyDoseAmt = 0;
     }*/
     //Serial.println("2.3.3");
    greenDailyDoseAmt = greenDailyDoseAmt+val;
     //Serial.println("2.3.4");
    retVal = greenDailyDoseAmt;
     //Serial.println("2.3.5");
    //String geenDailyDoseAmtStrr = String(greenDailyDoseAmt);
       //Serial.println("2.3.6");
  delay(100);
    //util->writeFile(SPIFFS,"/greenDailyDoseAmt.txt", geenDailyDoseAmtStrr.c_str());
    greenDailyDoseAmtStr = String(greenDailyDoseAmt);
    fileSystem.saveToFile("/greenDailyDoseAmt.txt", greenDailyDoseAmtStr.c_str());
     //Serial.println("2.3.7");
     //greenDailyDoseAmtStr.clear();
    // geenDailyDoseAmtStrr.clear();   
    Serial.print("Green Daily Dose Amt is: ");
    Serial.println(greenDailyDoseAmtStr);


    delay(500);
  }else if(color == 3){
     //Serial.println("2.4.1");
    //yellowDailyDoseAmtStr = readFile(SPIFFS, "/yellowDailyDoseAmt.txt");
    fileSystem.openFromFile("/yellowDailyDoseAmt.txt", yellowDailyDoseAmt);
     //Serial.println("2.4.2");
     /*if(yellowDailyDoseAmtStr != ""){
      yellowDailyDoseAmt = yellowDailyDoseAmtStr.toInt();
     }else{
      //TODO
      yellowDailyDoseAmt = 0;
     }*/
      //Serial.println("2.4.3");
   yellowDailyDoseAmt = yellowDailyDoseAmt+val;
     //Serial.println("2.4.4");
    retVal = yellowDailyDoseAmt;
     //Serial.println("2.4.5");
    //String yellowDailyDoseAmtStrr = String(yellowDailyDoseAmt);
     //Serial.println("2.4.6");
    delay(100);
  
    //util->writeFile(SPIFFS,"/yellowDailyDoseAmt.txt", yellowDailyDoseAmtStrr.c_str());
    yellowDailyDoseAmtStr = String(yellowDailyDoseAmt);
    fileSystem.saveToFile("/yellowDailyDoseAmt.txt", yellowDailyDoseAmtStr.c_str());
     //Serial.println("2.4.7");
     //yellowDailyDoseAmtStr.clear();
     //yellowDailyDoseAmtStrr.clear();
    Serial.print("Yellow Daily Dose Amt is: ");
    Serial.println(yellowDailyDoseAmtStr);
    delay(500);
  }else if(color == 4){
    //Serial.println("2.2.1");
    //blueDailyDoseAmtStr = readFile(SPIFFS, "/blueDailyDoseAmt.txt");
    fileSystem.openFromFile("/purpleDailyDoseAmt.txt", purpleDailyDoseAmt);
    //Serial.println("2.2.2");
    /*if(blueDailyDoseAmtStr != ""){
      blueDailyDoseAmt = blueDailyDoseAmtStr.toInt();
    }else{
      //TODO
      blueDailyDoseAmt = 0;
    }
    //Serial.println("2.2.3");*/
    purpleDailyDoseAmt = purpleDailyDoseAmt+val;
    //Serial.println("2.2.4");
    retVal = purpleDailyDoseAmt;
    //Serial.println("2.2.5");
    //String blueDailyDoseAmtStrr = String(blueDailyDoseAmt);
    //Serial.println("2.2.6");
    delay(100);
    //util->writeFile(SPIFFS,"/blueDailyDoseAmt.txt", blueDailyDoseAmtStrr.c_str());
    purpleDailyDoseAmtStr = String(purpleDailyDoseAmt);
    fileSystem.saveToFile("/purpleDailyDoseAmt.txt", purpleDailyDoseAmtStr.c_str());
     //Serial.println("2.2.7");
   Serial.print("Purple Daily Dose Amt is: ");
    Serial.println(purpleDailyDoseAmtStr);
    //blueDailyDoseAmtStr.clear();
    //blueDailyDoseAmtStrr.clear();
    delay(500);
  }
  return retVal;
}

bool writeDailyDoseAmtToDb(String color, int amt){
  bool retVal = true;
  //setDate();
  String month = "";
  if(mo == 1){
      month = "January";
  }else if(mo == 2){
    month = "February";
  }else if(mo == 3){
    month = "March";
  }else if(mo == 4){
    month = "April";
  }else if(mo == 5){
    month = "May";
  }else if(mo == 6){
    month = "June";
  }else if(mo == 7){
    month = "July";
  }else if(mo == 8){
    month = "August";
  }else if(mo == 9){
    month = "Septempber";
  }else if(mo == 10){
    month = "October";
  }else if(mo == 11){
    month = "November";
  }else if(mo == 12){
    month = "December";
  }
  String parentPath = "/Doser/"+color+"/Data/Year/"+yrStr+"/"+month+"/Day_"+daStr;
      //WebSerial.println(path);
 String childPath = "value";

  
  if(db->databaseReady()){
    retVal = db->putDailyDoseData(parentPath, childPath, amt);
  }else{
    db->initDb();
    retVal = db->putDailyDoseData(parentPath, childPath, amt);
  }
  String zero = String(0);
  if(color == "Blue"){
    fileSystem.saveToFile("/blueDailyDoseAmt.txt", zero.c_str());
  }else if(color == "Green"){
    fileSystem.saveToFile("/greenDailyDoseAmt.txt", zero.c_str());
  }else if(color == "Yellow"){
    fileSystem.saveToFile("/yellowDailyDoseAmt.txt", zero.c_str());
  }else if(color == "Purple"){
    fileSystem.saveToFile("/purpleDailyDoseAmt.txt", zero.c_str());
  }
  childPath.clear();
  return retVal;
}



/*
////////////////////////////////////////////////////////////////////
//
//  Fuction: readFile
//
//  Input:  SPIFFS,
//          char[]  filename path i.e. "/String.txt"
//
//  Output:  String of what was strored
//
//  Discription:  Stores a string in the /path in SPIFFS
//
/////////////////////////////////////////////////////////////////////
String readFile(fs::FS &fs, const char * path) {
  //Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if (!file || file.isDirectory()) {
    //Serial.println("- empty file or failed to open file");
    return String();
  }
  //Serial.println("- read from file:");
  String fileContent;
  while (file.available()) {
    fileContent += String((char)file.read());
  }
  //Serial.println(fileContent);
  file.close();
  return fileContent;
}

////////////////////////////////////////////////////////////////////
//
//  Fuction: writeFile
//
//  Input:  SPIFFS,
//          char[] filename path i.e. "/String.txt"
//          String to store
//
//  Output:  String of what was strored
//
//  Discription:  Stores a string in the /path in SPIFFS
//
/////////////////////////////////////////////////////////////////////
void writeFile(fs::FS &fs, const char * path, const char * message) {
  //Serial.printf("Writing file: %s\r\n", path);
  //Serial.print("path is : ");
  //Serial.println(path);
  //fs.remove(path);
  File file = fs.open(path, "w");
  if (!file) {
    Serial.println("- failed to open file for writing");
    //WebSerial.println("?EW"); //TODO change to right code
    return;
  }
  
  if (file.print(message)) {
    //Serial.println("- file written");
    if ((strcmp(path, "/timezone.txt") == 0)) {
      //Serial.println("Timezone changed!!!!!!!!11");
    }

  } else {
    Serial.println("- write failed");
  }
}

*/

bool writeDailyDosesToDb(){
  bool retVal = true;
  Serial.println("writing tooooooooo dbbbbbbbbbbbbbb");
  WebSerial.println("writing tooooooooo dbbbbbbbbbbbbbb");
  //TODO TODO TODO daily amount isn't adding
 // blueDailyDoseAmtStr = readFile(SPIFFS, "/blueDailyDoseAmt.txt");
  fileSystem.openFromFile("/blueDailyDoseAmt.txt", blueDailyDoseAmt);
  /*int bluAmt = 0;
  if(blueDailyDoseAmtStr != ""){
    //Serial.println("ERROR writeDaily");
    bluAmt = blueDailyDoseAmtStr.toInt();
  }*/
  retVal = writeDailyDoseAmtToDb("Blue", blueDailyDoseAmt);  //Blue has to be cap to match database
  //String reset = String(0);
  //writeFile(SPIFFS, "/blueDailyDoseAmt.txt",reset.c_str());
  //greenDailyDoseAmtStr = readFile(SPIFFS, "/greenDailyDoseAmt.txt");
  fileSystem.openFromFile("/greenDailyDoseAmt.txt", greenDailyDoseAmt);
 /* int grnAmt = 0;
  if(greenDailyDoseAmtStr != ""){
    grnAmt = greenDailyDoseAmtStr.toInt();
  }*/
  retVal = writeDailyDoseAmtToDb("Green", greenDailyDoseAmt);
  //writeFile(SPIFFS, "/greenDailyDoseAmt.txt",reset.c_str());
  //yellowDailyDoseAmtStr = readFile(SPIFFS, "/yellowDailyDoseAmt.txt");
  fileSystem.openFromFile("/yellowDailyDoseAmt.txt", yellowDailyDoseAmt);
  /*int ylwAmt = 0;
  if(yellowDailyDoseAmtStr != ""){
    ylwAmt = yellowDailyDoseAmtStr.toInt();
  }*/
  retVal = writeDailyDoseAmtToDb("Yellow", yellowDailyDoseAmt);
  //writeFile(SPIFFS, "/yellowDailyDoseAmt.txt",reset.c_str());
  //purpleDailyDoseAmtStr = readFile(SPIFFS, "/purpleDailyDoseAmt.txt");
  fileSystem.openFromFile("/purpleDailyDoseAmt.txt", purpleDailyDoseAmt);
  /*int purAmt = 0;
  if(purpleDailyDoseAmtStr != ""){
    purAmt = purpleDailyDoseAmtStr.toInt();
  }*/
  retVal = writeDailyDoseAmtToDb("Purple", purpleDailyDoseAmt);
 // writeFile(SPIFFS, "/purpleDailyDoseAmt.txt",reset.c_str());
  return retVal;
}

bool writeCalibrationToDb(int color, float amt){
  bool retVal = true;
  String parentPath = "";
  String childPath = "";

  if(color == 1){
    parentPath = "/Doser/Blue/Calibration";
    childPath = "secBluePerMl";
  }else if(color == 2){
    parentPath = "/Doser/Green/Calibration";
    childPath = "secBluePerMl";
  }else if(color == 3){
    parentPath = "/Doser/Yellow/Calibration";
    childPath = "secBluePerMl";
  }else if(color == 4){
    parentPath = "/Doser/Purple/Calibration";
    childPath = "secBluePerMl";
  }


  if(db->databaseReady()){
    retVal = db->putDailyDoseData(parentPath, childPath, amt);
  }else{
    db->initDb();
    retVal = db->putDailyDoseData(parentPath, childPath, amt);
  }
  childPath.clear();
  return retVal;
}

void setDate(){
  mySched->syncTime();
  yr = mySched->getCurrentYear();
  yr = yr;
  yrStr = String(yr);
  Serial.print("Year is: ");
  Serial.println(yrStr);
  mo = mySched->getCurrentMonth();
  Serial.print("Month is: ");
  Serial.println(mo);

  da = mySched->getCurrentDay();
  daStr = String(da-1);  //i need the day for save to db be the day before since i save at midnight
  Serial.print("YesterDay is: ");
  Serial.println(daStr);
 
  fileSystem.saveToFile("/curDay.txt", daStr);

}

//////////////////////////////////////////////////////////////
//                                                          //
//   sentHttp                                               //
//                                                          //
//   input: String message                                  //
//                                                          //
//   output: int (not used)                                 //
//                                                          //
//   description:  this uses the ifttt service to send      //
//                  push notifications                      //
//
//                                                          //
//////////////////////////////////////////////////////////////
int sendHttp(String event) {
  int ret = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print("*");
  }

  //Serial.println("");
  // Serial.println("WiFi connected");
  //Serial.println("IP address: ");
  // Serial.println(WiFi.localIP());


  //Serial.print("connecting to ");
  //Serial.println(host + url);
  //  client.setInsecure();
  if (!client.connect(host, httpsPort)) {

    Serial.println("connection failed");
    return 0;
  }
  //Serial.print("requesting URL: ");
  String iftt = "fBplW8jJqqotTqTxck4oTdK_oHTJKAawKfja-WlcgW-";//atoAwcUtil->readFile(SPIFFS, "/ifttt.txt");

  if (event == "Email") {
    //Serial.println("sending email");
    //url = "/trigger/" + event + "/with/key/"+iftt;//+",params={ \"value1\" : \""+iPAddress+"\", \"value2\" : \"02\", \"value3\" : \"03\" }";
    //Serial.println(url);
    //https://maker.ifttt.com/trigger/garage_deur/with/key/d56e34gf756/?value1=8&value2=5&value3=good%20morning
    //TESTING JSON CREATION
    String url = "/trigger/" + event + "/with/key/" + iftt;
    //Serial.println("Starting JSON");
    StaticJsonDocument<80> jdata;
    //        StaticJsonBuffer<69> jsonBuffer;
    String json = "{ \"value1\" : \"atoawc ip: " + iPAddress + "\", \"value2\" : \", atoawc hotspot pw: ato_awc_\", \"value3\" : \", doser hotspot pw : yourdoser\" }";
    auto error = deserializeJson(jdata, json);
    //        JsonObject& root = jsonBuffer.parseObject(json);
    //Serial.println("TESTING POST");

    client.print(String("POST ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 //"Connection: close\r\n" +
                 "Content-Type: application/json\r\n" +
                 "Content-Length: " + json.length() + "\r\n" +
                 "\r\n" + // This is the extra CR+LF pair to signify the start of a body
                 json + "\n");
  } else {

    //url = "/trigger/"+event+"/with/key/bOZMgCFy7Bnhee7ZRzyi19";
    url = "/trigger/" + event + "/with/key/" + iftt;

    //Serial.println(url);

    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "User-Agent: BuildFailureDetectorESP8266\r\n" +
                 "Connection: close\r\n\r\n");

    //Serial.println("request Sent");
  }
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      // Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');

  //Serial.println("reply was:");
  //Serial.println("==========");
  //Serial.println(line);
  //Serial.println("==========");
  //Serial.println("closing connection");
  ret = 1;
  return ret;
}

/*int getCurrentTime(String input){
  int retVal = 0;
    
    if(input == "year"){
      retVal = timeinfo.tm_year + 1900;
      //return year;
    }else if(input == "month"){
      retVal = timeinfo.tm_mon + 1;
      //return month;
    }else if(input == "day"){
      retVal = timeinfo.tm_mday;
      //return day;
    }else if(input == "hour"){
      retVal = timeinfo.tm_hour;
      //return hour;
    }else if (input == "minute"){
      retVal = timeinfo.tm_min;
      //return minute;
    }
    return retVal;
} */

/*bool getNTPtime(int sec) {

  {
    uint32_t start = millis();
    do {
      time(&now);
      localtime_r(&now, &timeinfo);
      //Serial.print("*");
      delay(10);
    } while (((millis() - start) <= (1000 * sec)) && (timeinfo.tm_year < (2016 - 1900)));
    if (timeinfo.tm_year <= (2016 - 1900)) return false;  // the NTP call was not successful
    //Serial.print("now ");
    //Serial.println(now);
    char time_output[30];
    //strftime(time_output, 30, "%a  %d-%m-%y %T", localtime(&now));
    //Serial.println(time_output);
    //Serial.println();
  }
  return true;
}

void setTheTime(tm localTime) {
  if (wifiConnected) {
    Serial.print("Day is: ");
    currentDay = localTime.tm_mday;
    Serial.println(currentDay);
    Serial.print("Month is: ");
    currentMonth = localTime.tm_mon + 1;
    Serial.println(currentMonth);
    Serial.print("Year is: ");
    currentYear = localTime.tm_year - 100;
    Serial.println(currentYear);
    Serial.print("Hour is: ");
    currentHour = localTime.tm_hour;
    Serial.println(currentHour);
    Serial.print("Minute is: ");
    currentMinute = localTime.tm_min;
    Serial.println(currentMinute);
    Serial.print("Second is: ");
    currentSecond = localTime.tm_sec;
    Serial.println(currentSecond);
    Serial.print(" Day of Week ");
    if (localTime.tm_wday == 0) Serial.println(7);
    else Serial.println(localTime.tm_wday);
  } else {
    RtcDateTime dt = Rtc.GetDateTime();
    currentMonth = dt.Month();
    Serial.print("RTC month is: ");
    Serial.print(currentMonth);
    currentDay = dt.Day();
    Serial.print("RTC day is: ");
    Serial.print(currentDay);
    currentYear = dt.Year();
    Serial.print("RTC year is: ");
    Serial.print(currentYear);
    currentHour = dt.Hour();
    Serial.print("RTC hour is: ");
    Serial.print(currentHour);
    currentMinute = dt.Minute();
    Serial.print("RTC minute is: ");
    Serial.print(currentMinute);
    currentSecond = dt.Second();
    Serial.print("RTC second is: ");
    Serial.print(currentSecond);
  }
}

void setRtcTime() {
  RtcDateTime currentTime = RtcDateTime(currentYear, currentMonth, currentDay, currentHour, currentMinute, 0);  //define date and time object
  Rtc.SetDateTime(currentTime);                                                                                 //configure the RTC with object
} */

void doseBlue(int evt){
  //Serial.println("In Dose BLUE");
  if(!greenDosing && !yellowDosing && !purpleDosing && !blueDosed){
    WebSerial.print("B");
    blueDosing = doser->dose(1);
    bluePumpPending = false;
    if(!blueDosing){         
      //Serial.println("b");
      WebSerial.print("b");
      blueDosed = true;
      if(!bluePumpPending && !greenPumpPending && !yellowPumpPending && !purplePumpPending){
        //Serial.println("setFlagindosingblue");
        mySched->setFlag(evt,0);
        thisIsAPumpEvent = false;
        alreadyReset = true;
        //Serial.print("Resetting evn ");
        //Serial.println(evt);
      }
      int err = doser->getErrorCode();
      if(err == 99){
        WebSerial.println("D_B_tl");
      }
      int amt = addDailyDoseAmt(1,doser->getDoseRun());
      int rst = sendHttp("Blue_Dosing");
      //TODO remove
      
      //mySched->resetFlag(1);
      delay(1000); // have to delay here so doesn't reset 15 min timer wait a minute
    }
  }else{
	if(!blueDosing) {
		//Serial.println("bp");
		bluePumpPending = true;
	}
  }
            //TODO remove this it is at midnight just testing a every 15min
  
}

void doseGreen(int evt){
           //Serial.println("Thirty Minutes");
          if(!blueDosing && !yellowDosing && !purpleDosing && !greenDosed){
              WebSerial.print("G");
              greenDosing = doser->dose(2);
              greenPumpPending = false;
              //Serial.println("Shi9");
            if(!greenDosing){         
             // mySched->setFlag(evt,0);
              Serial.println("g");
              WebSerial.print("g");
              greenDosed = true;
      if(!bluePumpPending && !greenPumpPending && !yellowPumpPending && !purplePumpPending){
        //Serial.println("setflaggreen");
        mySched->setFlag(evt,0);
        thisIsAPumpEvent = false;
        alreadyReset = true;
       }
               int err = doser->getErrorCode();
              if(err == 99){
                WebSerial.println("D_G_tl");
              }
              int amt = addDailyDoseAmt(2,doser->getDoseRun());
              int rst = sendHttp("Green_Dosing");
           delay(1000); // have to delay here so doesn't reset 15 min timer wait a minute
            }
          }else{
            if(!greenDosed) {
            	greenPumpPending = true;
            	Serial.println("gp");
            }
          }
}

void doseYellow(int evt){
  if(!blueDosing && !greenDosing && !purpleDosing && !yellowDosed){
    WebSerial.print("Y");
    yellowDosing = doser->dose(3);
    yellowPumpPending = false;
    if(!yellowDosing){         
      //mySched->setFlag(evt,0);
      Serial.println("y");
      WebSerial.print("y"); 
      yellowDosed = true;
      if(!bluePumpPending && !greenPumpPending && !yellowPumpPending && !purplePumpPending){
        //Serial.println("setflagYellow");
        mySched->setFlag(evt,0);
        thisIsAPumpEvent = false;
        alreadyReset = true;
       }
      int err = doser->getErrorCode();
      if(err == 99){
       WebSerial.println("D_Y_tl");
      }
    int amt = addDailyDoseAmt(3,doser->getDoseRun());
  int rst = sendHttp("Yellow_Dosing");
    }
  
  
  delay(1000);
}else{
  if(!yellowDosed) {
	  yellowPumpPending = true;
	  Serial.println("yp");
  }
}
}

void dosePurple(int evt){
  //Serial.println("In Dose PURPLE");
  if(!greenDosing && !yellowDosing && !blueDosing && !purpleDosed){
    //Serial.println("C");
    WebSerial.print("P");
    purpleDosing = doser->dose(4);
    purplePumpPending = false;
    if(!purpleDosing){         
      Serial.println("p");
      WebSerial.print("p");
      purpleDosed = true;
      if(!bluePumpPending && !greenPumpPending && !yellowPumpPending && !purplePumpPending){
        //Serial.println("setFlagindosingPurple");
        mySched->setFlag(evt,0);
        thisIsAPumpEvent = false;
        alreadyReset = true;
        //Serial.print("Resetting evn ");
        //Serial.println(evt);
      }
      int err = doser->getErrorCode();
      if(err == 99){
        WebSerial.println("D_P_tl");
      }
      int amt = addDailyDoseAmt(4,doser->getDoseRun());
      int rst = sendHttp("Purple_Dosing");
      //TODO remove
      
      //mySched->resetFlag(1);
      delay(1000); // have to delay here so doesn't reset 15 min timer wait a minute
    }
  }else{
    if(!purpleDosed) {
    	purplePumpPending = true;
    	Serial.println("pp");
    }
  }
            //TODO remove this it is at midnight just testing a every 15min
  
}
/*void dosePurple(int evt){
  //Serial.println("Two Hour");
  if(!blueDosing && !yellowDosing && !greenDosing && !purpleDosed){
    WebSerial.print("P");
    purpleDosing = doser->dose(4);
    purplePumpPending = false;
    if(!purpleDosing){         
      //mySched->setFlag(evt,0);
        Serial.println("p");
        WebSerial.print("p");
        purpleDosed = true;
        if(!bluePumpPending || !greenPumpPending || !yellowPumpPending || !purplePumpPending){
          Serial.println("setflagpurple");
          mySched->setFlag(evt,0);
        }
        int err = doser->getErrorCode();
        if(err == 99){
          WebSerial.println("D_P_tl");
        }
        int amt = addDailyDoseAmt(4,doser->getDoseRun());
        int rst = sendHttp("Purple_Dosing");
        delay(1000); // have to delay here so doesn't reset 15 min timer wait a minute
      }
    }else{
      Serial.println("pp");
      purplePumpPending = true;
    }
}*/

bool notDosing(){
  bool retVal = true;
  if(!blueDosing && !greenDosing && !yellowDosing && ! purpleDosing){
    retVal = true;
  }else{
    retVal = false;
  }
  return retVal;
}

void checkDosingSched(int i){
    if(db->isThisEventPumpSet(i, 0)){
      thisIsAPumpEvent = true;
      doseBlue(i);
    }
    if (db->isThisEventPumpSet(i, 1)){
      thisIsAPumpEvent = true;
      doseGreen(i);
    }
    if(db->isThisEventPumpSet(i, 2)){
      thisIsAPumpEvent = true;
      doseYellow(i);
    }
    if (db->isThisEventPumpSet(i, 3)){
      //Serial.println("B+");
      thisIsAPumpEvent = true;
      dosePurple(i);
    }
    if(notDosing() && !alreadyReset){
      mySched->setFlag(i,0);
    }

  if(i == 8 && !midnightDone){
    midnightDone = true;
    setDate();
    mySched->setFlag(i,0);
    mySched->updateMyTime();
    writeDailyDosesToDb();
    WebSerial.print("saved to db");
  }
  if(!thisIsAPumpEvent){
    mySched->setFlag(i,0);
  }
}

