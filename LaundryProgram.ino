/*  
 *  ================ DRYER number 1 ================ 
 *   
 *  Machine Type   = 0(WASHER), 1(DRYER)
 *  Machine Number = 1-6 (6 WASHER & 6 DRYER)
 *  Machine Status = true(ON), false(OFF)
 *  Machine ID     = Unique Number (1 - 12), different machine different ID
 *  ID             = Auto generate from system
 *  
 *  THIS MACHINE = {
 *  "machine_type"   : 1,
 *  "machine_number" : 1,  
 *  "machine_status" : false,
 *  "machine_id"     : 1,
 *  "id"             : 1 }
 * 
 */

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "EEPROM.h"
#include "FS.h"
#include "SPIFFS.h"

#define FORMAT_SPIFFS_IF_FAILED true

#define URL_UPDATE "http://192.168.1.6:8000/update?sl_id=1"
#define URL_GET "http://192.168.1.6:8000/machine-id?sl_id=1"

#define LED_WIFI    19
#define RESET_BTN   34
#define PIN_MACHINE 25
#define PIN_MACHINE1 26
#define PIN_MACHINE2 27
#define PIN_MACHINE3 14
#define TON_MACHINE 1 //minutes

#define STS_ADDR    0
#define MINUTE_ADDR 1
#define SECOND_ADDR 2

AsyncWebServer server(80);

HTTPClient http;

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "Dryer 1";
const char* password = "12345678";

const char* FileSSID = "/ssid.txt";
const char* FilePASS = "/pass.txt";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    .center { margin-left:auto; margin-right:auto; font-family: "Trebuchet MS", Arial, Helvetica, sans-serif;
    margin-top: 35px;border-collapse: collapse; width: 80%; text-align: center; }
    input[type=submit] { background-color: #D91C1C; color: white; padding: 10px 20px; margin: 5px 0;
          border: none; border-radius: 3px; cursor: pointer; }
    input[type=submit]:hover { background-color: #EB606C; }
    h2{margin-top:35px; text-align:center; background-color: #FFFFFF;}
  </style><h2>LAUNDRY : Washer 1</h2></head><body>
  <table class="center">
    <form name="loginForm" action="/get">
      <tr><th>Username</th><td><input type="text" id="ssid" name="ssid"></td></tr>
      <tr><td></td></tr>
      <tr><th>Password</th><td><input type="text" id="pass" name="pass"></td></tr>
      <tr><td></td></tr>
      <tr><th colspan="2"><input type="submit" value="Submit"></th></tr>
    </form></table>
</body></html>)rawliteral";

// LOGIN
String UserName, PassWord;
bool OpenWeb = false, disconnect = true;
int firstWait = 1;
int * waiting = &firstWait;

// TIMER
unsigned long prevTime, currentTime;
byte detik, menit, machineSts;

//SERVER
int httpResponseCode;
bool machineState = false, prevMachineState = false;
bool setMachineON = false, machineOn = false;
bool updateServer = false;

//RESET BUTTON
unsigned long btnTime, prevBtnTime;
bool paksaNyala = false;

/* 
 * @brief     : EEPROM Initialization Function
 *
 * @details   : This function used to start EEPROM function, read the latest data that saved in EEPROM
 * 
 * @param     : none
 * 
 * @retval    : none
 */
void EEPROM_Init(){
  if(!EEPROM.begin(3)){
    Serial.println("Failed to init EEPROM");
  }

  machineSts = EEPROM.read(STS_ADDR);
  menit = EEPROM.read(MINUTE_ADDR);
  detik = EEPROM.read(SECOND_ADDR);

  Serial.print("EEPROM : "); Serial.print(machineSts);
  Serial.print("\t"); Serial.print(menit); Serial.print("\t"); Serial.println(detik);

  // If machineSts = 1, menit != 0, and detik != 0
  // It means, in the past, machine was on but suddenly power is off, so I will continue to turn on the machine.
  // But the machine is not tirggered anymore. So, setMachine is TRUE to continue the timer and
  // machineOn is FALSE to disable trigger function
  if(machineSts == 1 && ((menit > 0) || (detik > 0))){
    setMachineON = true;
    machineOn = false;
  }
}

/*
 * @brief   : Machine Trigger function

 * @details : This function used to trigger Washer/ Dryer by turn on PIN_MACHINE(GPIO25) for 50 millisecond and then turn off.
 *            To trigger the Washer/Dryer, machineOn variable must be worth TRUE. machineOn will be TRUE if 
 * 
 * @param   : none
 * 
 * @retval  : none
 */
void MACHINE_on(){
  if(machineOn){
    Serial.println("Mesin Dinyalakan");
    
    digitalWrite(PIN_MACHINE, HIGH);
    delay(50);
    digitalWrite(PIN_MACHINE, LOW);
    delay(100);

    machineOn = false;
  }
}

void IRAM_ATTR isr() {
  btnTime = millis();
  if(btnTime - prevBtnTime >= 5000){
    paksaNyala = true;
    prevBtnTime = millis();
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_MACHINE, OUTPUT);
  digitalWrite(PIN_MACHINE, LOW);
  delay(100);

  pinMode(LED_WIFI, OUTPUT);

  pinMode(RESET_BTN, INPUT);
  attachInterrupt(RESET_BTN, isr, FALLING);

  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  Serial.println("Initalized");

  WIFI_Connection();

  EEPROM_Init();
}

void loop() {
  // Check this controller is connectea to Access Point or not
  // If not connect, Turn off led indicator, change this module to Access Poin
  // you can access 192.168.4.1 to input new SSID and PASSWORD of available WiFi
  if(WiFi.status() != WL_CONNECTED){
    digitalWrite(LED_WIFI, LOW);
    disconnect = true;
    WIFI_Connection();
  }
  // If this module connected to WiFi, get update status machine from server and then trigger the machine if status machine change from 0 to 1
  else{
    // Get update data machine from server
    SERVER_getJsonResponse();

    // setMachineOn will be true if status machine in the server change from 0 to 1, it means the controller must activate this Wassher/Dryer
    if(setMachineON){
      MACHINE_on();

      currentTime = millis();
      if((currentTime - prevTime) >= 900){
        detik++;
        if(detik == 60){
            menit++;
            detik = 0;
        }
        // Save MACHINE STATUS, MINUTE, SECOND to EEPROM
        EEPROM.write(STS_ADDR, machineState);
        EEPROM.write(MINUTE_ADDR, menit);
        EEPROM.write(SECOND_ADDR, detik);
        EEPROM.commit();
        
        // If the timer reach TON_MACHINE : Washer/Dryer is finished
        if(menit == TON_MACHINE){
          updateServer = true;
          setMachineON = false;
        }
        Serial.print("Menit : ");
        Serial.print(menit);
        Serial.print("\t");
        Serial.print("Detik : ");
        Serial.println(detik);
        prevTime = millis();
      }
    }

    // Update status Washer/Dryer to server after Washer/Dryer is finished
    if(updateServer){
      Serial.println("Update Status Machine on Server");
      if(SERVER_Update(0)){
        menit = 0;
        detik = 0;
        updateServer = false;

        // Update data machine(Machine State = 0, Minute = 0, Second = 0) to EEPROM
        SERVER_getJsonResponse();
        EEPROM.write(STS_ADDR, machineState);
        EEPROM.write(MINUTE_ADDR, 0);
        EEPROM.write(SECOND_ADDR, 0);
        EEPROM.commit();
      }
    }
  }

  // paksaNyala is flag that used to indicate the RESET_BTN button is being pressed or not
  if(paksaNyala){
    machineOn = true;
    MACHINE_on();
    paksaNyala = false;
  }
}