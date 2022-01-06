
void mountFS() {

  SPIFFS.begin (true);
  
  if (SPIFFS.begin()) {
    Serial.println("\nFile system mounted!");
    } 
  else {
    Serial.println("Failed to mount file system.\n");
    }
}
    
    
void parseJson(const char* filename, DynamicJsonDocument& json) {
  if (SPIFFS.exists(filename)) { // file exists, reading and loading
    File file = SPIFFS.open(filename, "r");   // Open file for reading
    DeserializationError error = deserializeJson(json, file);
    if (error) Serial.println("Failed to read json file");
    file.close();
  } else {
    Serial.println("Json file doesn't exist");
  }
}
