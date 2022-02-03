#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "FS.h"
#include "SPIFFS.h"

#define FORMAT_SPIFFS_IF_FAILED true

AsyncWebServer server(80);

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "laundry";
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

String UserName, PassWord;
bool OpenWeb = false, disconnect = true;
int firstWait = 1;
int * waiting = &firstWait;

unsigned long prevTime, currentTime;
unsigned int detik, menit;


void setup() {
  Serial.begin(115200);

  pinMode(2, OUTPUT);

  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("SPIFFS Mount Failed");
        return;
  }
  Serial.println("Initalized");

  WIFI_Connection();
}

void loop() {
  if(WiFi.status() != WL_CONNECTED){
    digitalWrite(2, LOW);
    disconnect = true;
    WIFI_Connection();
  }
  Serial.println("Masuk Loop");
  delay(1000);
    // currentTime = millis();
    // if((currentTime - prevTime) >= 1000){
    //     detik++;
    //     if(detik == 60){
    //         menit++;
    //         detik = 0;
    //     }
    //     Serial.print(menit);
    //     Serial.print("\t");
    //     Serial.println(detik);
    //     prevTime = millis();
    // }
}