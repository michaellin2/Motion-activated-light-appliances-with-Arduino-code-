#include "Arduino.h"
#include <WiFiClient.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
 
#define EEPROM_SIZE 12
// const char* ssid = "BT-6ZAGW5";
// const char* password = "PvhViqdkLc7nCR";
const char* ssid = "10DC Hyperoptic 1Gb Fibre 2.4Ghz";
const char* password = "Michael0408.";
WebServer server(80);
String apiName = "";
int address = 0;
// Serving Hello world


void setRoom() {
    String postBody = server.arg("plain");
    DynamicJsonDocument doc(512);
    DeserializationError parsingError = deserializeJson(doc, postBody);

    //if there is error in the recieved json object
    if (parsingError) {
        server.send(400, F("text/html"),"Error in parsin json body! <br>");
 
    } 
    //if the object is able to be parsed
    else {
      //create a json object
        JsonObject postObj = doc.as<JsonObject>();
        //if the method is post
        if (server.method() == HTTP_POST) {
          //if the post request has whats needed
            if (postObj.containsKey("name")) {
 
                Serial.println(F("done."));
                String lolLmao= doc["name"];
                apiName=lolLmao;
                Serial.println(lolLmao);
                EEPROM.begin(EEPROM_SIZE);
                for(int i=0;i<apiName.length();i++){
                  EEPROM.write(address,apiName[i]);
                  address++;
                }

                EEPROM.write(address,'\0');
                EEPROM.end();
                //creating data to send as a response to the main thing 
                DynamicJsonDocument doc(512);
                //populating the json Object
                doc["status"] = "OK";
                String buf;
                //serialising data
                serializeJson(doc, buf);
                
                server.send(201, F("application/json"), buf);
                
 
            }else {
                DynamicJsonDocument doc(512);
                doc["status"] = "KO";
                doc["message"] = F("No data found, or incorrect!");
 
                Serial.print(F("Stream..."));
                String buf;
                serializeJson(doc, buf);
 
                server.send(400, F("application/json"), buf);
                Serial.print(F("done."));
            }
        }
    }
}
 
void getRoom(){
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.read(address);
  char c;
  int currAddr = 0;
  String data = "";
  while(c != '\0'){
    c = EEPROM.read(currAddr);
    if(c!='\0'){
      data+=c;
    }
    currAddr++;
  }
  Serial.println(data);
  EEPROM.end();
  server.send(200,F("text/plain"),data);
    
}
// Define routing
void restServerRouting() {
    server.on("/", HTTP_GET, []() {
        server.send(200, F("text/html"),
            F("Welcome to the REST Web Server"));
    });
    // handle post request
    server.on(F("/setRoom"), HTTP_POST, setRoom);
    server.on(F("/getRoom"), HTTP_GET, getRoom);
}
 
// Manage not found URL
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}
 
void setup(void) {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
 
  // Activate mDNS this is used to be able to connect to the server
  // with local DNS hostmane esp8266.local
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
 
  // Set server routing
  restServerRouting();
  // Set not found response
  server.onNotFound(handleNotFound);
  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
}