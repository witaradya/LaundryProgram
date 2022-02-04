void WIFI_notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

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