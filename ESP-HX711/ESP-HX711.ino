/*********************************************************************

D4 - Hx711 data, D5 Hx711 clock

https://github.com/bogde/HX711
How to calibrate your load cell
Call set_scale() with no parameter.
Call tare() with no parameter.
Place a known weight on the scale and call get_units(10).
Divide the result in step 3 to your known weight. You should get about the parameter you need to pass to set_scale().
Adjust the parameter in step 4 until you get an accurate reading.



*********************************************************************/

#include "HX711.h"
#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <string.h>
#include <ArduinoOTA.h>

#ifndef APSSID
#define APSSID "myScale1"
#define APPSK  "elacSym123"
#endif

const char *ssid = APSSID;
const char *password = APPSK;
String mystring;
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(8000);


// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 4; //Hardwired
const int LOADCELL_SCK_PIN = 5; //Hardwired
//const float calibrationFactor=226.626;
const float calibrationFactor=1;
unsigned long previousMillis = 0;     
long timeoutinterval = 1500; 


HX711 scale;










/* Set these to your desired credentials. */


static const char PROGMEM MANIFEST_JSON[] = R"rawliteral(
{
"name": "myROVER",
"short_name": "myrover",
"description": "A simple rover controller app.",
"display": "standalone",
"scope": "/",
"orientation":  "landscape"
}
)rawliteral";

static const char PROGMEM CHECK_SENSORS[] = R"rawliteral(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no">
</head>
<body>
</body>
</html>
)rawliteral";
static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=0">
      <link rel="manifest" href="manifest.json">
        <style>
          /*        disable scrolling    */
          html {
            margin: 0;
            padding: 0;
            overflow: hidden;
          }
        
        body {
          position: absolute;
          width: 100%;
          height: 100%;
          overflow: auto;
          margin: 0px;
          
        }
        
        * {
          margin: 0;
          padding: 0;
          box-sizing: border-box;
        }
        
        
        @media only screen and (max-height: 444px) {
          .horizontalslider {
            position: relative;
            height: 10%;
          }
        }
        
                  
        
                    .mybuttons {
  display: inline-block;
  padding: 15px 25px;
  font-size: 24px;
  cursor: pointer;
  text-align: center;
  text-decoration: none;
  outline: none;
  color: #fff;
  background-color: #4CAF50;
  border: none;
  border-radius: 15px;
  box-shadow: 0 9px #999;
}

.mybuttons:hover {background-color: #3e8e41}

.mybuttons:active {
  background-color: #3e8e41;
  box-shadow: 0 5px #666;
  transform: translateY(4px);
}
        .col-1 {
          position: absolute;
          width: 24%;
          right: 75%
        }
        
        .col-2 {
          position: absolute;
          width: 46%;
          right: 38%;
        }
        
        .col-3 {
          position: absolute;
          width: 38%;
          right: 0;
        }
        #websocketstatus[type="radio"]:disabled {
          
          color: aqua;
          background-color: aqua;
          border-color: aquamarine;
          
          
        }
        </style>
        <title>myScale1</title>
        </head>
  <body>
        <div class = "col-1">
    <label for="websocketstatus">Connected</label>
            <input type="radio" id="websocketstatus" disabled>
        <br><br><br>
        <button class="mybuttons" id="connectWS" onclick='doConnect()'>Connect
        </button>
            <br><br><br>
                    <button class="mybuttons" id="closeWS" onclick='doClose()'>Close
        </button>
            <br><br><br>    
            <button class="mybuttons" id="reloadfromsource" onclick='location.reload()'>Reload
        </button>
            <br><br><br>    
            <button class="mybuttons" id="tareScale" onclick='tareScale()'>Tare
        </button>
             <br><br><br>    
            <button class="mybuttons" id="calibrateScale" onclick='calibrateScale()'>Calibrate
        </button>
            </div>
        <button class = "col-3 mybuttons" id ="serialConsole"></button>
  </body>
  <script>
    
    
  function tareScale(){
        doSend("tare");
    }
  function handlewebsocketstatus(){
    if (iswebsocketconnected){
      document.getElementById("websocketstatus").checked=true;
    }
    else
    {
      document.getElementById("websocketstatus").checked=false;
    }
  }       
  
  
  var websock;
    var calibrationFactor = 1;
    function calibrateScale(){
        calibrationFactor= prompt("Calibration Factor", calibrationFactor);
    }
  var iswebsocketconnected = false;
  function doConnect() {
      // websock = new WebSocket('ws://' + window.location.hostname + ':8000/');
      websock = new WebSocket('ws://192.168.4.1:8000/');
    
      websock.onopen = function(evt) {
        console.log('websock open');
        iswebsocketconnected = true;
        handlewebsocketstatus();
        
      }
      
      websock.onclose = function(evt) {
        iswebsocketconnected = false;
        console.log('websock close');
        handlewebsocketstatus();
          //clear all the intervals - doesn't work. still dealing with "ghost" setIntervals
          
      }
      websock.onerror = function(evt) {
        console.log(evt);
      }
      websock.onmessage = function(evt) {
          //console.log(evt); 
          console.log(evt.data);
          document.getElementById("serialConsole").innerHTML=Math.round(evt.data*calibrationFactor);
          }
            //parse_incoming_websocket_messages(evt.data);
        }
  
  function doClose() {
      //detach steering servo
        //attachordetachservos("w","d");
        
        websock.close();
  }
  function doSend(message) {
    if (iswebsocketconnected == true) {
      if (websock.readyState == websock.OPEN) {
        websock.send(message);
          //console.log(message);
      } else {
        
        console.log("websocket is in an indeterminate state");
      }
    }
  }
  </script>
</html>


)rawliteral";


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
//Serial1.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
switch(type) {
case WStype_DISCONNECTED:
//Serial1.printf("[%u] Disconnected!\r\n", num);
break;
case WStype_CONNECTED:
{
IPAddress ip = webSocket.remoteIP(num);
//Serial1.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
//itoa( cm, str, 10 );
//  webSocket.sendTXT(num, str, strlen(str));
}
break;
case WStype_TEXT:
{
//Serial1.printf("[%u] get Text: %s\r\n", num, payload);
//Send whatever comes on the WS to Atmega.
char *mystring1 = (char *)payload;
mystring = mystring1;
if (strcmp(mystring1,"tare") == 0)
{
  scale.tare();
  }

}
break;
case WStype_BIN:
//Serial1.printf("[%u] get binary length: %u\r\n", num, length);
break;
default:
//Serial1.printf("Invalid WStype [%d]\r\n", type);
break;
}
}

void handleRoot()
{
server.sendHeader("Cache-Control","max-age=604800");
server.send(200, "text/html", INDEX_HTML);

}
void handleIndex()
{
//server.send_P(200, "text/html", INDEX_HTML);
server.sendHeader("Cache-Control","max-age=604800");
server.send(200, "text/html", INDEX_HTML);

}


void handleManifest()
{
server.sendHeader("Cache-Control","max-age=604800");
server.send(200, "application/json", MANIFEST_JSON);

}
void handleCheck()
{
server.sendHeader("Cache-Control","max-age=604800");
server.send(200, "text/html", CHECK_SENSORS);

}

void handleNotFound()
{
String message = "File Not Found\n\n";
message += "URI: ";
message += server.uri();
message += "\nMethod: ";
message += (server.method() == HTTP_GET)?"GET":"POST";
message += "\nArguments: ";
message += server.args();
message += "\n";
for (uint8_t i=0; i<server.args(); i++){
message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
}
server.send(404, "text/plain", message);
}









void setup()   {
Serial.begin(9600);
scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
//scale.set_scale(211.8*10.7);                      // this value is obtained by calibrating the scale with known weights; see above and the https://github.com/bogde/HX711 README for details
scale.set_scale(226.626); 
scale.tare(); 

Serial.println("All good in setup");

/***************** AP mode*******************/

WiFi.softAP(ssid, password);
WiFi.printDiag(Serial);
IPAddress myIP = WiFi.softAPIP();


/***********************************************/

/*****************Client Mode******************/
/*
WiFiMulti.addAP(ssid, password);
while(WiFiMulti.run() != WL_CONNECTED) {
Serial1.print(".");
delay(100);
}
Serial1.println("");
Serial1.print("Connected to ");
Serial1.println(ssid);
Serial1.print("IP address: ");
Serial1.println(WiFi.localIP());
*/
/**********************************************/

server.on("/", handleRoot);
server.on("/index.html",handleIndex);
server.on("/manifest.json",handleManifest);
server.on("/check.html",handleCheck);
server.onNotFound(handleNotFound);
//  server.onNotFound([]() {                              // If the client requests any URI
//    if (!handleFileRead(server.uri()))                  // send it if it exists
//      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
//  });

server.begin();

webSocket.begin();
webSocket.onEvent(webSocketEvent);


/* ************SPIFFS********************* */
//  if (SPIFFS.begin()){Serial1.println("file system mounted");};
//
//  //Open the "Save.txt" file and check if we were saving before the reset happened
//  File q = SPIFFS.open("/Save.txt", "r");
//  if (q.find("Y")){saveData=true;}
//  q.close();

/*********************************************/

/* ************OTA********************* */

// Port defaults to 8266
// ArduinoOTA.setPort(8266);

// Hostname defaults to esp8266-[ChipID]
// ArduinoOTA.setHostname("myesp8266");

// No authentication by default
// ArduinoOTA.setPassword((const char *)"123");

ArduinoOTA.onStart([]() {
//Serial1.println("Start");
});
ArduinoOTA.onEnd([]() {
//Serial1.println("\nEnd");
});
ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
//Serial1.printf("Progress: %u%%\r", (progress / (total / 100)));
});
ArduinoOTA.onError([](ota_error_t error) {
/*
Serial1.printf("Error[%u]: ", error);
if (error == OTA_AUTH_ERROR) Serial1.println("Auth Failed");
else if (error == OTA_BEGIN_ERROR) Serial1.println("Begin Failed");
else if (error == OTA_CONNECT_ERROR) Serial1.println("Connect Failed");
else if (error == OTA_RECEIVE_ERROR) Serial1.println("Receive Failed");
else if (error == OTA_END_ERROR) Serial1.println("End Failed");
*/
});
ArduinoOTA.begin();

/****************************************************/  

}


void loop() {

webSocket.loop();
server.handleClient();
ArduinoOTA.handle();

unsigned long currentMillis = millis();

if (currentMillis - previousMillis >= timeoutinterval) {
previousMillis = currentMillis;

if (scale.is_ready()) {


float reading = (scale.get_units(10))*calibrationFactor; //With scale.set_scale(226.626) - this gives accurate up to 1 deca gram (10 grams). 500 gram water bottle values are 50.07 to 50.32
//float readinginGs = reading*10;
////float readinginKGs = round(readinginGs/1000); 
//float readinginLBs = readinginGs/453.592;// 4545
////Serial.print("reading in lbs= ");
////Serial.println(readinginLBs);
//
//int intermediatevalueholder = (int) (round(readinginGs/100)); //70992.87 becomes  710
//int leftofdecimal = intermediatevalueholder/10; //71
////Serial.println(leftofdecimal);
//int rightofdecimal = (intermediatevalueholder)-(leftofdecimal*10);
Serial.println(reading);
char datastr[16];
dtostrf(reading,10, 4, datastr);
webSocket.broadcastTXT(datastr, strlen(datastr));
}
}
}
