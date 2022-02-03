String FILE_read(fs::FS &fs, const char * path){
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

void FILE_write(fs::FS &fs, const char * path, String message){
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