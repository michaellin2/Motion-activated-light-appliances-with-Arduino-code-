#include "Arduino.h"
#include <WiFiClient.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <analogWrite.h>
#include <WiFiManager.h>
#include "DHT.h"
#define tempSensorPin 27
#define sensorPin 26
#define red 25
#define green 23
#define blue 32
#define EEPROM_SIZE 1
//#define EEPROM_SIZE 12

// initialize web server ports
WebServer server(80);

uint8_t color = 0;
uint8_t brightness = 255;
uint32_t R, G, B;
const boolean invert = true;
boolean tempButton = false;
boolean rgbButton = false;

String tempButtonString = "";
String rgbButtonString = "";
int data = 0;
int address = 0;
float temperature = 0;

DHT dht(tempSensorPin, DHT22);
WiFiManager wifimanager;

// reset the wifi
void resetWifi()
{
  wifimanager.resetSettings();
  server.send(200, F("application/json"), "");
}

// if the motion sensor detected movement, turn on the light
void getRGBLight()
{
  if (digitalRead(sensorPin) == HIGH)
  {
    RGB(apiName, 255);
    ledcWrite(0, R);
    ledcWrite(1, G);
    ledcWrite(2, B);
  }
  else
  {
    ledcWrite(0, 0);
    ledcWrite(1, 0);
    ledcWrite(2, 0);
  }
}

// post request for mobile application to send request which
// enables the user to manually change the rgb light
void setRgbController()
{
  String postBody = server.arg("plain");
  DynamicJsonDocument doc(512);
  DeserializationError parsingError = deserializeJson(doc, postBody);

  // if there is error in the recieved json object
  if (parsingError)
  {
    server.send(400, F("text/html"), "Error in parsin json body! <br>");
  }
  // if the object is able to be parsed
  else
  {
    // create a json object
    JsonObject postObj = doc.as<JsonObject>();
    // if the method is post
    if (server.method() == HTTP_POST)
    {
      // if the post request has whats needed
      if (postObj.containsKey("color"))
      {
        int Color = doc["color"];
        data = Color;

        // creating data to send as a response to the main thing
        DynamicJsonDocument doc(512);
        // populating the json Object
        doc["status"] = "OK";
        String buf;
        // serialising data
        serializeJson(doc, buf);

        server.send(201, F("application/json"), buf);
      }
      else
      {
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

void getRgbController()
{
  String hel = "";
  hel.concat(apiName);
  server.send(200, F("text/plain"), hel);
}

//check if the RGB button is turned off, if its turned off, then turn it on
void turnOnRGBButton()
{
  if (rgbButton == false)
  {
    rgbButton = true;
  }
  rgbButtonString.concat(rgbButton);
  server.send(200, F("text/plain"), rgbButtonString);
}


//check if the RGB button is turned on, if its turned on, then turn it off
void turnOffRGBButton()
{
  if (rgbButton == true)
  {
    rgbButton = false;
  }
  rgbButtonString.concat(rgbButton);
  server.send(200, F("text/plain"), rgbButtonString);
}


//check if the temperature button is turned off, if its turned off, then turn it on
void turnOnTempButton()
{
  if (tempButton == false)
  {
    tempButton = true;
  }
  tempButtonString.concat(tempButton);
  server.send(200, F("text/plain"), tempButtonString);
}


//check if the temperature button is turned on, if its turned on, then turn it off
void turnOffTempButton()
{
  if (tempButton == true)
  {
    tempButton = false;
  }
  tempButtonString.concat(tempButton);
  server.send(200, F("text/plain"), tempButtonString);
}

//function to get the rgb color
void RGB(uint8_t hue, uint8_t brightness)
{
  uint16_t scaledHue = (hue * 6);
  uint8_t segment = scaledHue / 256; // segment 0 to 5 around the
                                     // color wheel
  uint16_t segmentOffset =
      scaledHue - (segment * 256); // position within the segment

  uint8_t complement = 0;
  uint16_t prev = (brightness * (255 - segmentOffset)) / 256;
  uint16_t next = (brightness * segmentOffset) / 256;

  if (invert)
  {
    brightness = 255 - brightness;
    complement = 255;
    prev = 255 - prev;
    next = 255 - next;
  }

  switch (segment)
  {
  case 0: // red
    R = brightness;
    G = next;
    B = complement;
    break;
  case 1: // yellow
    R = prev;
    G = brightness;
    B = complement;
    break;
  case 2: // green
    R = complement;
    G = brightness;
    B = next;
    break;
  case 3: // cyan
    R = complement;
    G = prev;
    B = brightness;
    break;
  case 4: // blue
    R = next;
    G = complement;
    B = brightness;
    break;
  case 5: // magenta
  default:
    R = brightness;
    G = complement;
    B = prev;
    break;
  }
}

//if the motion sensor detected any motion, turn the rgb light on, else turn the light off
void motionDetect()
{
  String motion = "";
  motion.concat(digitalRead(sensorPin));
  server.send(200, F("text/plain"), motion);
  if (digitalRead(sensorPin) == HIGH)
  {
    for (color = 0; color < 255; color++)
    {
      RGB(color, brightness);
      ledcWrite(0, R);
      ledcWrite(1, G);
      ledcWrite(2, B);
      delay(10);
    }
  }
  else
  {
    ledcWrite(0, 0);
    ledcWrite(1, 0);
    ledcWrite(2, 0);
  }
}

//get the current temperature and check if its higher than 22 or lower than 5.
//If it matches any of the condition, then change the light color to specified color.
//The most important condition is that it must detected motion before running the temperature sensor. 
void getTemperature()
{
  temperature = dht.readTemperature();
  String temp = "";
  temp.concat(temperature);
  if (digitalRead(sensorPin) == HIGH && temperature > 22)
  {
    ledcWrite(2, B);
  }
  else if (digitalRead(sensorPin) == HIGH && temperature < 5)
  {
    ledcWrite(0, R);
  }
  else
  {
    motionDetect();
  }
  server.send(200, F("text/plain"), temp);
}

//Wifi configuration
void wifiManagement()
{
  // explicitly set mode, esp defaults to STA+AP
  WiFi.mode(WIFI_STA); 

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result
  bool res;
  res = wifimanager.autoConnect("AutoConnectAP", "password"); 

  //checking for connection failure
  if (!res)
  {
    Serial.println("Failed to connect");
  } else
  {
    Serial.println("You have connected:)");
  }

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// void setRoom() {
//     String postBody = server.arg("plain");
//     DynamicJsonDocument doc(512);
//     DeserializationError parsingError = deserializeJson(doc, postBody);
//     //if there is error in the recieved json object
//     if (parsingError) {
//         server.send(400, F("text/html"),"Error in parsin json body! <br>");
//     }
//     //if the object is able to be parsed
//     else {
//       //create a json object
//         JsonObject postObj = doc.as<JsonObject>();
//         //if the method is post
//         if (server.method() == HTTP_POST) {
//           //if the post request has whats needed
//             if (postObj.containsKey("name")) {
//                 Serial.println(F("done."));
//                 String lolLmao= doc["name"];
//                 apiName=lolLmao;
//                 Serial.println(lolLmao);
//                 EEPROM.begin(EEPROM_SIZE);
//                 for(int i=0;i<apiName.length();i++){
//                   EEPROM.write(address,apiName[i]);
//                   address++;
//                 }
//                 EEPROM.write(address,'\0');
//                 EEPROM.end();
//                 //creating data to send as a response to the main thing
//                 DynamicJsonDocument doc(512);
//                 //populating the json Object
//                 doc["status"] = "OK";
//                 String buf;
//                 //serialising data
//                 serializeJson(doc, buf);
//                 server.send(201, F("application/json"), buf);
//             }else {
//                 DynamicJsonDocument doc(512);
//                 doc["status"] = "KO";
//                 doc["message"] = F("No data found, or incorrect!");
//                 Serial.print(F("Stream..."));
//                 String buf;
//                 serializeJson(doc, buf);
//                 server.send(400, F("application/json"), buf);
//                 Serial.print(F("done."));
//             }
//         }
//     }
// }

// void getRoom(){
//   EEPROM.begin(EEPROM_SIZE);
//   EEPROM.read(address);
//   char c;
//   int currAddr = 0;
//   String data = "";
//   while(c != '\0'){
//     c = EEPROM.read(currAddr);
//     if(c!='\0'){
//       data+=c;
//     }
//     currAddr++;
//   }
//   Serial.println(data);
//   EEPROM.end();
//   server.send(200,F("text/plain"),data);
// }
// Define routing

//creating request to send to server
void restServerRouting()
{
  server.on("/", HTTP_GET, []()
            { server.send(200, F("text/html"),
                          F("Welcome to the REST Web Server")); });
  // handle post request
  server.on(F("/setRGB"), HTTP_POST, setRgbController);
  server.on(F("/getRGB"), HTTP_GET, getRgbController);
  server.on(F("/getTemperature"), HTTP_GET, getTemperature);
  server.on(F("/getMotion"), HTTP_GET, motionDetect);
  server.on(F("/turnOnTempButton"), HTTP_GET, turnOnTempButton);
  server.on(F("/turnOffTempButton"), HTTP_GET, turnOffTempButton);
  server.on(F("/turnOnRGBButton"), HTTP_GET, turnOnRGBButton);
  server.on(F("/turnOffRGBButton"), HTTP_GET, turnOffRGBButton);
  server.on(F("/resetWifi"), HTTP_GET, resetWifi);
}

// Manage not found URL
void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(void)
{
  //setup the arudino
  Serial.begin(115200);

  //setup the temperature sensor
  dht.begin();

  // set up the motion sensor
  pinMode(sensorPin, INPUT);

  // set up the rgb color
  ledcSetup(0, 5000, 8);
  ledcAttachPin(red, 0);

  ledcSetup(1, 5000, 8);
  ledcAttachPin(green, 1);

  ledcSetup(2, 5000, 8);
  ledcAttachPin(blue, 2);

  //calling the wifimanagement function to configure the device
  wifiManagement();

  // Set server routing
  restServerRouting();
  // Set not found response
  server.onNotFound(handleNotFound);
  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void)
{
  server.handleClient();
  //if the temperature button and RGB button arent pressed, then run the rgb light normally
  //else if temperature button is pressed, run the getTemperature function,
  //else if the rgb button is pressed, run the getrgblight function. 
  if (tempButton == false)
  {
    if (rgbButton == false)
    {
      motionDetect();
    }
    else
    {
      getRGBLight();
    }
  }
  else
  {
    getTemperature();
  }
}