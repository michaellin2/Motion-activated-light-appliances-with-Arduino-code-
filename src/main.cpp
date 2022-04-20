#include "Arduino.h"
#include <WiFiClient.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <analogWrite.h>

#include"DHT.h"
#define tempSensorPin 27
#define sensorPin 26
#define red 25
#define green 23
#define blue 32
uint8_t color =0;
uint8_t brightness = 255;
uint32_t R,G,B;
const boolean invert =true;

#define EEPROM_SIZE 12


// const char* ssid = "BT-6ZAGW5";
// const char* password = "PvhViqdkLc7nCR";
const char* ssid = "10DC Hyperoptic 1Gb Fibre 2.4Ghz";
const char* password = "Michael0408.";
WebServer server(80);
String apiName = "";
int address = 0;
// Serving Hello world

float temperature = 0;
DHT dht(tempSensorPin,DHT22);

void RGB(uint8_t hue, uint8_t brightness)
{
    uint16_t scaledHue = (hue * 6);
    uint8_t segment = scaledHue / 256; // segment 0 to 5 around the
                                            // color wheel
    uint16_t segmentOffset =
      scaledHue - (segment * 256); // position within the segment

    uint8_t complement = 0;
    uint16_t prev = (brightness * ( 255 -  segmentOffset)) / 256;
    uint16_t next = (brightness *  segmentOffset) / 256;

    if(invert)
    {
      brightness = 255 - brightness;
      complement = 255;
      prev = 255 - prev;
      next = 255 - next;
    }

    switch(segment ) {
    case 0:      // red
        R = brightness;
        G = next;
        B = complement;
    break;
    case 1:     // yellow
        R = prev;
        G = brightness;
        B = complement;
    break;
    case 2:     // green
        R = complement;
        G = brightness;
        B = next;
    break;
    case 3:    // cyan
        R = complement;
        G = prev;
        B = brightness;
    break;
    case 4:    // blue
        R = next;
        G = complement;
        B = brightness;
    break;
   case 5:      // magenta
    default:
        R = brightness;
        G = complement;
        B = prev;
    break;
    }
}

void motionDetect(){
  if (digitalRead(sensorPin)==HIGH){
    for(color=0;color<255;color++){
      RGB(color,brightness);
      ledcWrite(0,R);
      ledcWrite(1,G);
      ledcWrite(2,B);
      delay(10);
    }
  }
}

void getTemperature(){
  temperature = dht.readTemperature();
  if(temperature>22){
    ledcWrite(2,B);
  }
  else if(temperature<5){
    ledcWrite(0,R);
  }
  else{
    ledcWrite(0,R);
    ledcWrite(1,G);
    ledcWrite(2,B);
  }
}

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
  //start the temperature sensor
  dht.begin();
  //motion sensor
  pinMode(sensorPin,INPUT);
  //rgb color
  ledcSetup(0,5000,8);
  ledcAttachPin(red,0);

  ledcSetup(1,5000,8);
  ledcAttachPin(green,1);

  ledcSetup(2,5000,8);
  ledcAttachPin(blue,2);

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
  motionDetect();
}