void WIFI_notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

/*
 * @brief     : WIFI_WebLogin handle Form Login using WEB Server Based(192.168.4.1)

 * @details   : Activate web server(this module change to web server based) for show Login form
 *              This form can be accsess by enterin 192.168.4.1 and then you must enter SSID and PASS
 * 
 * @param     : none
 * 
 * @retval    : none 
 */
void WIFI_WebLogin(){
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
      FILE_write(SPIFFS, "/ssid.txt", fsSSID);
      FILE_write(SPIFFS, "/pass.txt", fsPASS);
      OpenWeb = false;
      *waiting = 0;
    }
  });
  server.onNotFound(WIFI_notFound);
  server.begin();
}

/*
 * @brief      : WIFI Connection Function

 * @details    : Handle if this controler not connect to WiFi or first initial to connect WiFi
                 Get SSID and PASS from ssid.txt and pass.txt via SPIFFS
 *               1. Turn off WiFi Led Indicator
 *               2. Read SSID and PASS that saved in ssid.txt and pass.txt
 *               3. Connect to WiFi based on the SSID and PASS
 *               4. If failed connect to SSID, this module change to Access Poin and you must input new and correct SSID and PASS via web
 *                  visit 192.168.4.1 to input new SSID and PASS
 *               5. Your new SSID and PASS will be saved in ssid.txt and pass.txt via SPIFFS
 *               6. And then this module will automatically connect to newest SSID and PASS
 * 
 * @param      : none
 * 
 * @retval     : none
 */
void WIFI_Connection(){
  while(disconnect){
    if(!OpenWeb){
      digitalWrite(LED_WIFI, LOW);
      UserName = FILE_read(SPIFFS, "/ssid.txt");
      PassWord = FILE_read(SPIFFS, "/pass.txt");
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
          digitalWrite(LED_WIFI, HIGH);
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
      WIFI_WebLogin();
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