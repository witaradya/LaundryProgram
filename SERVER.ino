void SERVER_Get(String JSONMessage){
    DynamicJsonDocument doc(2048);
    deserializeJson(doc,JSONMessage);
    Serial.println(doc.size());
    
    bool machineSts = doc["machine_status"];
    Serial.println(machineSts);
}

void SERVER_getJsonResponse(){
    http.begin(URL_GET); //Specify the URL
    
    int httpCode = http.GET();
    if (httpCode > 0) {
      //Serial.println("GET status mesin"); 
      prevMachineState = machineState;
      DynamicJsonDocument doc(2048);
      deserializeJson(doc,http.getString());

      machineState = doc["machine_status"];
      // Serial.print("Mesin status : ");
      // Serial.println(machineState);
      if(machineState && !prevMachineState) {
        setMachineON = true;
        machineOn = true;
        // Serial.println("Nyalakan Mesin ...");
      }
        // String payload = http.getString();
        // Serial.println(httpCode);
        // Serial.println(payload);
        // SERVER_Get(payload);
    }
  
    else {
      Serial.println("Error on HTTP request");
    }
  
    http.end(); //Free the resources
}

bool SERVER_Update(bool stsMachine){
  http.begin(URL_UPDATE);

  http.addHeader("accept", "application/json");
  http.addHeader("Content-Type", "application/json");
  
  if(stsMachine) httpResponseCode = http.PUT("{\"machine_status\":\"true\"}");
  else httpResponseCode = http.PUT("{\"machine_status\":\"false\"}"); 
    
  if(httpResponseCode>0){
    String response = http.getString();   
    Serial.println(httpResponseCode);
    Serial.println(response);          
  }
  else{
    Serial.print("Error on sending PUT Request: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();

  if(httpResponseCode > 0) return true;
  else return false;
}