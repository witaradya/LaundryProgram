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

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS &fs, const char * path){
  String result;
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
      Serial.println("- failed to open file for reading");
      return "salah";
  }

  Serial.println("- read from file:");
  while(file.available()){
      char a = file.read();
      result += a;
  }
  Serial.println(result);
  return result;
  file.close();
}

void writeFile(fs::FS &fs, const char * path, String message){
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}

void WEB_Login(){
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String fsSSID, fsPASS;
    if ((request->hasParam("ssid")) && (request->hasParam("pass"))) {
      fsSSID = request->getParam("ssid")->value();
      fsPASS = request->getParam("pass")->value();
    }
    if(fsSSID == "" || fsPASS == ""){
      request->send(200, "text/html", "ULANGI! USERNAME / PASSWORD KOSONG!<br><a href=\"/\">Return to Home Page</a>");
    }
    else {
      request->send(200, "text/html", "SSID ("+ fsSSID + ") dan PASS (" + fsPASS +") BERHASIL DISIMPAN!");
      writeFile(SPIFFS, "/ssid.txt", fsSSID);
      writeFile(SPIFFS, "/pass.txt", fsPASS);
      OpenWeb = false;
      *waiting = 0;
    }
  });
  server.onNotFound(notFound);
  server.begin();
}

void WIFI_Connection(){
  while(disconnect){
    if(!OpenWeb){
      digitalWrite(2, LOW);
      UserName = readFile(SPIFFS, "/ssid.txt");
      PassWord = readFile(SPIFFS, "/pass.txt");
      if(UserName == "" || PassWord == "" || UserName == " " || PassWord == " "){
        OpenWeb = true;
        Serial.println("Koneksi gagal");
      }
      else{
        Serial.println("Koneksi Sukses, Lanjut ke wifi");
        delay(1000);
        WiFi.mode(WIFI_STA);
        WiFi.begin(UserName.c_str(), PassWord.c_str());
        if (WiFi.waitForConnectResult() != WL_CONNECTED) {
          OpenWeb = true;
          Serial.println("WiFi Failed!");
        }
        else{
          digitalWrite(2, HIGH);
          Serial.println();
          Serial.print("IP Address: ");
          Serial.println(WiFi.localIP());
          disconnect = false;
        }      
      }    
    }
    else{
      *waiting = 1;
      server.begin();
      Serial.println("Masuk Web");
      WEB_Login();
      Serial.println(firstWait);
      while(firstWait){
        if(WiFi.status() == WL_CONNECTED){
          *waiting = 0;
          OpenWeb = false;
          //disconnect = false;
        }
        Serial.println(firstWait);
        delay(1000);
      }
    }
  }
}


void setup() {
  Serial.begin(115200);

//   pinMode(2, OUTPUT);

//   if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
//         Serial.println("SPIFFS Mount Failed");
//         return;
//   }
//   Serial.println("Initalized");

//   WIFI_Connection();
}

void loop() {
//   if(WiFi.status() != WL_CONNECTED){
//     digitalWrite(2, LOW);
//     disconnect = true;
//     WIFI_Connection();
//   }
//   Serial.println("Masuk Loop");
//   delay(1000);
    currentTime = millis();
    if((currentTime - prevTime) >= 1000){
        detik++;
        if(detik == 60){
            menit++;
            detik = 0;
        }
        Serial.print(menit);
        Serial.print("\t");
        Serial.println(detik);
        prevTime = millis();
    }
}