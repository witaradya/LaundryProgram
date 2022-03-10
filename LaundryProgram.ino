/*
    ================ WASHER number 1 ================

    Machine Type   = 0(WASHER), 1(DRYER)
    Machine Number = 1-6 (6 WASHER & 6 DRYER)
    Machine Status = true(ON), false(OFF)
    Machine Grade  = true(Titan), false(Giant)
    Machine Price  = Rp ....
    Machine ID     = Unique Number (1 - 12), different machine different ID
    ID             = Auto generate from system

    THIS MACHINE = {
    "machine_type"   : 0,
    "machine_number" : 1,
    "machine_status" : false,
    "machine_grade"  : false,
    "machine_price"  : "1000"
    "machine_id"     : 1,
    "id"             : 1 }

*/


/*
 * Choose one to activate 1 device that will you use
 */
#define WASHER_1  //id = 1
//#define WASHER_2  //id = 2
//#define WASHER_3  //id = 3
//#define DRYER_1   //id = 4
//#define DRYER_2   //id = 5
//#define DRYER_3   //id = 6

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "EEPROM.h"
#include "FS.h"
#include "SPIFFS.h"

#define FORMAT_SPIFFS_IF_FAILED true

#ifdef WASHER_1
  #define URL_UPDATE "http://192.168.1.30:8000/update?sl_id=1"
  #define URL_GET "http://192.168.1.30:8000/machine-id?sl_id=1"
  const char* ssid = "WASHER 1";

#elif WASHER_2
  #define URL_UPDATE "http://192.168.1.30:8000/update?sl_id=2"
  #define URL_GET "http://192.168.1.30:8000/machine-id?sl_id=2"
  const char* ssid = "WASHER 2";

#elif WASHER_3
  #define URL_UPDATE "http://192.168.1.30:8000/update?sl_id=3"
  #define URL_GET "http://192.168.1.30:8000/machine-id?sl_id=3"
  const char* ssid = "WASHER 3";

#elif DRYER_1
  #define URL_UPDATE "http://192.168.1.30:8000/update?sl_id=4"
  #define URL_GET "http://192.168.1.30:8000/machine-id?sl_id=4"
  const char* ssid = "DRYER 1";

#elif DRYER_2
  #define URL_UPDATE "http://192.168.1.30:8000/update?sl_id=5"
  #define URL_GET "http://192.168.1.30:8000/machine-id?sl_id=5"
  const char* ssid = "DRYER 2";

#elif DRYER_3
  #define URL_UPDATE "http://192.168.1.30:8000/update?sl_id=6"
  #define URL_GET "http://192.168.1.30:8000/machine-id?sl_id=6"
  const char* ssid = "DRYER 3";
#endif


#define LED_WIFI      19
#define RESET_BTN     34
#define PIN_MACHINE   25
#define PIN_MACHINE1  26
#define PIN_MACHINE2  27
#define PIN_MACHINE3  14
#define TON_MACHINE   3  //minutes

#define STS_ADDR    0
#define MINUTE_ADDR 1
#define SECOND_ADDR 2

AsyncWebServer server(80);

HTTPClient http;

TaskHandle_t Task1;

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
  </style><h2>LAUNDRY</h2></head><body>
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
bool paksaNyala = false;
uint8_t lastState = HIGH;
uint8_t currentState;
unsigned long pressTime, rilisTime;

//EEPROM


/*
   @brief     : EEPROM Initialization Function

   @details   : This function used to start EEPROM function, read the latest data that saved in EEPROM

   @param     : none

   @retval    : none
*/
void EEPROM_Init() {
  if (!EEPROM.begin(3)) {
    Serial.println("Failed to init EEPROM");
  }

  machineSts = EEPROM.read(STS_ADDR);
  menit = EEPROM.read(MINUTE_ADDR);
  detik = EEPROM.read(SECOND_ADDR);

  // Serial.print("EEPROM : "); Serial.print(machineSts);
  // Serial.print("\t"); Serial.print(menit); Serial.print("\t"); Serial.println(detik);

  // If machineSts = 1, menit != 0, and detik != 0
  // It means, in the past, machine was on but suddenly power is off, so I will continue to turn on the machine.
  // But the machine is not tirggered anymore. So, setMachine is TRUE to continue the timer and
  // machineOn is FALSE to disable trigger function
  if (machineSts == 1 && ((menit > 0) || (detik > 0))) {
    setMachineON = true;
    machineOn = false;
  }

  // If machineSts = 1, menit = TON_MACHINE, and deetik >= 0
  // This state is used to cover when timer is on point and micom cannot update to server
  // then machine and controler turn off, even tough controler still update status
  // So, this state will continue update status machine to off after machine and controler get supply
  if ((machineSts == 1) && (menit == TON_MACHINE) && (detik >= 0)) {
    updateServer = true;
    setMachineON = false;
  }
}

/*
   @brief   : Machine Trigger function

   @details : This function used to trigger Washer/ Dryer by turn on PIN_MACHINE(GPIO25) for 50 millisecond and then turn off.
              To trigger the Washer/Dryer, machineOn variable must be worth TRUE. machineOn will be TRUE if

   @param   : none

   @retval  : none
*/
void MACHINE_on() {
  if (machineOn) {
    Serial.println("Mesin Dinyalakan");

    digitalWrite(PIN_MACHINE, HIGH);
    delay(50);
    digitalWrite(PIN_MACHINE, LOW);
    delay(100);

    machineOn = false;
  }
}

/*
   @brief   : Button By Pass Function

   @details : Used to handle when button byPass is pressed in 5 second or more

   @param   : none

   @retval  : none
*/
void Button_ByPass() {
  currentState = digitalRead(RESET_BTN);
  if (lastState == HIGH && currentState == LOW)pressTime = millis();
  else if (lastState == LOW && currentState == HIGH) {
    rilisTime = millis();
    if ((rilisTime - pressTime) > 5000) {
      paksaNyala = true;
      Serial.println("DIPENCET !!!");
    }
  }
  lastState = currentState;
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_MACHINE, OUTPUT);
  digitalWrite(PIN_MACHINE, LOW);
  delay(100);

  pinMode(LED_WIFI, OUTPUT);

  pinMode(RESET_BTN, INPUT);

  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  Serial.println("Initalized");

  WIFI_Connection();

  EEPROM_Init();

  xTaskCreatePinnedToCore(
    Task1code,   /* Task function. */
    "Task1",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Task1,      /* Task handle to keep track of created task */
    0);          /* pin task to core 0 */
  delay(500);

}

// Devide 2 task in 2 core
// Task1(void loop) have a task to control WiFi connection
// Task2 control timer when machine turn on
void Task1code( void * pvParameters ) {
  while (1) {
    // setMachineOn will be true if status machine in the server change from 0 to 1, it means the controller must activate this Wassher/Dryer
    if (setMachineON) {
      MACHINE_on();

      currentTime = millis();
      if ((currentTime - prevTime) >= 1900) {
        detik++;
        if (detik == 30) {
          menit++;
          detik = 0;
        }
        // Save MACHINE STATUS, MINUTE, SECOND to EEPROM
        EEPROM.write(STS_ADDR, machineState);
        EEPROM.write(MINUTE_ADDR, menit);
        EEPROM.write(SECOND_ADDR, detik);
        EEPROM.commit();

        // If the timer reach TON_MACHINE : Washer/Dryer is finished
        if (menit == TON_MACHINE) {
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
    vTaskDelay(100);
  }
}

void loop() {
  // Check button is pressed or not
  Button_ByPass();

  // Check this controller is connectea to Access Point or not
  // If not connect, Turn off led indicator, change this module to Access Poin
  // you can access 192.168.4.1 to input new SSID and PASSWORD of available WiFi
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_WIFI, LOW);
    disconnect = true;
    WIFI_Connection();
  }
  // If this module connected to WiFi, get update status machine from server and then trigger the machine if status machine change from 0 to 1
  else {
    // Get update data machine from server
    SERVER_getJsonResponse();

    // Update status Washer/Dryer to server after Washer/Dryer is finished
    if (updateServer) {
      Serial.println("Update Status Machine on Server");
      if (SERVER_Update(0)) {
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
  if (paksaNyala && setMachineON) {
    machineOn = true;
    MACHINE_on();
    paksaNyala = false;
  }
}
