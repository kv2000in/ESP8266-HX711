/*
19:33:54.810 -> Flash real id:   00144020
19:33:54.810 -> Flash real size: 1048576 bytes
19:33:54.810 -> 
19:33:54.810 -> Flash ide  size: 1048576 bytes
19:33:54.810 -> Flash ide speed: 40000000 Hz
19:33:54.810 -> Flash ide mode:  DIO
19:33:54.846 -> Flash Chip configuration ok

SPIFFS 31 char filename limit

struct FSInfo {
    size_t totalBytes;
    size_t usedBytes;
    size_t blockSize;
    size_t pageSize;
    size_t maxOpenFiles;
    size_t maxPathLength;
};

Keep track of available flash size. 
Ability to find/connect/register clients - give it names. 
Client wakes up - reads sensor data  - sleeps with putting HX711 in sleep mode as well. 
Sort out tare, what to store, taring/zeroing etc.

*/

#include <ESP8266WiFi.h>
//#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <ESP8266WebServer.h>
//#include <ESP8266mDNS.h>
#include "FS.h"
#include <ArduinoOTA.h>
#include <string.h>
#include <espnow.h>
char mydata[64]; 
char macaddr[6];
long zerofactor=-817214;
int calibrationfactor=1;
int mytarereading=0;
bool dataavailable = false;

void onDataReceiver(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(mydata,incomingData,64);
  memcpy(macaddr,mac,6);
dataavailable = true;
}

#define MAX_STRING_LEN  64



// Function to return a substring defined by a delimiter at an index
char* subStr (char* str, char *delim, int index) {
  char *act, *sub, *ptr;
  static char copy[MAX_STRING_LEN];
  int i;

  // Since strtok consumes the first arg, make a copy
  strcpy(copy, str);

  for (i = 1, act = copy; i <= index; i++, act = NULL) {
     //Serial.print(".");
     sub = strtok_r(act, delim, &ptr);
     if (sub == NULL) break;
  }
  return sub;

}
//char *record1 = "one two three";
//char *record2 = "Hello there friend";
//Serial.println(subStr(record1, " ", 1));
//output = one




const char *ssid = "ESP";
const char *password = "monte123";



ESP8266WebServer server(80);
//WiFiServer tcpserver(9999);//TCPserver
WebSocketsServer webSocket = WebSocketsServer(81);

boolean saveData = false;
boolean broadcast = false;


unsigned long currESPSecs, currTime,timestamp, zeroTime;

static const char PROGMEM INDEX_HTML[] = R"rawliteral(

<!DOCTYPE html>
<html>
  <head>
    <meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">
      <title>Transducer Height Project</title>
      <style>
        "body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }"
        </style>
      <style>
        .ledon {
          align-items: flex-start;
          text-align: center;
          cursor: default;
          color: buttontext;
          background-color: buttonface;
          box-sizing: border-box;
          padding: 2px 6px 3px;
          border-width: 2px;
          border-style: outset;
          border-color: buttonface;
          border-image: initial;
          text-rendering: auto;
          color: initial;
          letter-spacing: normal;
          word-spacing: normal;
          text-transform: none;
          text-indent: 0px;
          text-shadow: none;
          display: inline-block;
          text-align: start;
          margin: 0em;
          font: 11px system-ui;
          
        }
      
      .ledoff {
        background-color: #1ded4d;
        
      }
      </style>
      <script>
        var websock;
        var boolClientactive;
        var boolHasZeroBeenDone = false;
        
        function start() {
          websock = new WebSocket('ws://' + window.location.hostname + ':81/');
          websock.onopen = function(evt) { console.log('websock open');
            
            
            
          };
          websock.onclose = function(evt) { console.log('websock close'); };
          websock.onerror = function(evt) { console.log(evt); };
          websock.onmessage = function(evt) {
            console.log(evt);

          };
          
          
        }
      

      function doSend(message)
      {
        console.log("sent: " + message + '\n');
        /* writeToScreen("sent: " + message + '\n'); */
        websock.send(message);
      }
      function fnButton(com){
        
        doSend("COMMAND-"+com);
        
      };
      function download(file){
        
        
      }
      function zero(){
        if (boolClientactive){
          fnButton('ZERO');
          boolHasZeroBeenDone = true; 
        }
        else {
          alert("No client data - ?Client not connected"); 
        }
      }
      
      function startsave()
      {
        if(boolHasZeroBeenDone){
          var decision = confirm("This will overwrite previous data.\n Press OK to proceed, Cancel to abort and Download data");
          if (decision){
            
            fnButton('STARTSAVE');
            /*Disable start save button and enable stop save button
             Ideally - server should send whether it is currently saving data or not
             */
            document.getElementById("btSTARTSAVE").disabled=true;
            document.getElementById("btZERO").disabled=true;
            document.getElementById("btSTOPSAVE").disabled=false;
          } else {
            /*user selected cancel - let's redirect user to download the data*/
            
          }
        } else { alert("Set Zero Before Saving")}
      }
      function stopsave(){
        
        fnButton('STOPSAVE');
        document.getElementById("btSTARTSAVE").disabled=false;
        document.getElementById("btZERO").disabled=false;
        document.getElementById("btSTOPSAVE").disabled=true;
      }
      function sendTimestamp(){
        
        /*Send timestamp in milliseconds when connected */
        var d = new Date();
        var n = (d.getTime()/1000);
        doSend("TIME-"+n);
      }
      </script>
  </head>
  <body onpageshow="javascript:start();">
    <!--
Server keeps the calibration data (based on MAC addresses)
Server keeps track of raw values at the time of Tare
Server returns data as 


-->
        
    <div><b>Digital Scale Server</b></div> 
    <label>Available Scales</label><list id = "listofsensornodes">
        
        <button>Assign</button>
        <button>Zero</button>
        <button>Plot</button>
        <Label>Value since zero</Label> <input>
        </list>
    <strong><tspan  id="tspanDistance1">0</tspan> <text> &nbsp;(cm)</text>
    </strong> <button class = "ledon" id ="serverblink"></button>
    <div><b>Height of Client</b></div>
    
    <strong><tspan  id="tspanDistance2">0</tspan> <text> &nbsp;(cm)</text>
    </strong> <button class = "ledon" id ="clientblink"></button> 
    <br>
    <button id = "btZERO" onclick="zero();">SET ZERO</button>
    <button id = "btSTARTSAVE" onclick="startsave();">Start Save</button>
    <button id = "btSTOPSAVE" onclick="stopsave();">Stop Save</button>
    <br><br><br>
    <a href="/Data.txt">Download Data</a>
    <br><br>
    <a href="/Zero.txt">Download Zero File</a>
    <br><br>
    <button id = "btCLOSEWS" onclick="websock.close();">Close Websocket</button>
    
  </body>
  
</html>



)rawliteral";




void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      //itoa( cm, str, 10 );
      //  webSocket.sendTXT(num, str, strlen(str));
      }
      break;
    case WStype_TEXT:
    {
      //Serial.printf("[%u] get Text: %s\r\n", num, payload);

      //Payload will be in the form of 3 alphapets and 3 digits
      //DATA-105 means it is Data and value is 105
      //COMMAND-ZERO means Command ZERO
      //TIME-12345678 means epoch timestamp in seconds
      
      char *split1,*split2 ;
     char *mystring = (char *)payload;
      split1=subStr(mystring, "-", 1);
      split2=subStr(mystring, "-", 2);
      
      if (strcmp(split1,"COMMAND") == 0)
        {
          //Serial.println("Received Command");
          //Serial.println(split2);
          if (strcmp(split2,"ZERO") == 0){
           // Serial.println("Zero command received");
//            if (!clientHeight){
//              Serial.println("No client data");
//              } else {
//            ZeroOffset=serverHeight-clientHeight;
//            zeroTime=timestamp+((millis()/1000)-currESPSecs);
//            //Serial.println(currTime);
//            File f = SPIFFS.open("/Zero.txt", "w");
//            f.print("Time:");
//            f.print(zeroTime);
//            f.print("\n");
//            f.print("ZeroOffset:");
//            f.print(ZeroOffset);
//            f.print("\n");
//            f.close();
//            //Reset the currESPsecs to current millis at the zerotime
//            //So the data save time will be zero time + secs elapsed since zero time
//            currESPSecs = millis()/1000;
//            
//            }
          } else if (strcmp(split2,"STARTSAVE") == 0){
            
            if (!saveData){
              saveData=true;
            File f = SPIFFS.open("/Data.txt", "w");
            f.close();
            //Create a file which keeps log of current saving process
            File s = SPIFFS.open("/Save.txt", "w");
            s.print("Y");
            s.close();
            }
            
            } 

            else if (strcmp(split2,"STOPSAVE") == 0) 
            {
              if (saveData){saveData=false;
                File s = SPIFFS.open("/Save.txt", "w");
                s.print("N");
                s.close();
              }
              }

             
            else if (strcmp(split2,"RUSAVING") == 0) 
            {
              if (saveData){
                webSocket.sendTXT(num, "Y");
                } else {
                  
                webSocket.sendTXT(num, "N");  
                  }
              }

        
        } 
      else if (strcmp(split1,"DATA") == 0)
        {
          //Serial.println("Received Data");
          //Serial.println(split2);
          //clientHeight=atoi(split2);
          if (saveData){
          //currTime=timestamp+((millis()/1000)-currESPSecs);
          currTime=((millis()/1000)-currESPSecs);
          File f = SPIFFS.open("/Data.txt", "a");
          f.print(currTime);
          f.print(":");
          f.print("C:");
          //f.print(clientHeight);
          f.print("\n");
          f.close();
          }
          if(broadcast){
           webSocket.broadcastTXT(split2, strlen(split2));
            } else {
              //If not broadcast - send to browser client num =1
              webSocket.sendTXT(1,split2, strlen(split2));
              
              }
          
        } 
      else if (strcmp(split1,"TIME") == 0)
        {
          //Serial.println("Received TimeStamp");
          //Serial.println(split2);
          timestamp=atol(split2);
          //currTime=split2;
          currESPSecs = millis()/1000;
          //Serial.println(timestamp);
          //Serial.println(currESPSecs);
        }
      else 
      {
          Serial.printf("Unknown-");
          Serial.printf("[%u] get Text: %s\r\n", num, payload);
          // send data to all connected clients
          //webSocket.broadcastTXT(payload, length);
      }
    }
    // clientHeight=atoi((const char *)payload);
     // Serial.println((const char *)payload);
     // itoa( cm, str, 10 );
      // webSocket.sendTXT(0, str, strlen(str));
      break;
    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\r\n", num, length);
      hexdump(payload, length);

      // echo data back to browser
      webSocket.sendBIN(num, payload, length);
      break;
    default:
      Serial.printf("Invalid WStype [%d]\r\n", type);
      break;
  }
}

void handleRoot()
{
  server.send_P(200, "text/html", INDEX_HTML);
}




//String getContentType(String filename); // convert the file extension to the MIME type
bool handleFileRead(String path);       // send the right file to the client (if it exists)
//String getContentType(String filename) { // convert the file extension to the MIME type
//  if (filename.endsWith(".html")) return "text/html";
//  else if (filename.endsWith(".css")) return "text/css";
//  else if (filename.endsWith(".js")) return "application/javascript";
//  else if (filename.endsWith(".ico")) return "image/x-icon";
//  return "text/plain";
//}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  //Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";         // If a folder is requested, send the index file
  //String contentType = getContentType(path);            // Get the MIME type
  if (SPIFFS.exists(path)) {                            // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it
    //size_t sent = server.streamFile(file, contentType); // And send it to the client
    size_t sent = server.streamFile(file, "text/plain"); // And send it to the client
    file.close();                                       // Then close the file again
    return true;
  }
  //Serial.println("\tFile Not Found");
  return false;                                         // If the file doesn't exist, return false
}

int fractionoccupiedspaceonfilesystem(){
FSInfo fs_info;
SPIFFS.info(fs_info);
return fs_info.usedBytes/fs_info.totalBytes; 
  
  }

void handledatafromnodes(char *sendermacaddress,char *datatobeprocessed){
  //Data arrives as "-780305.00|3300" - excluding quotes
//Send data as is to websocket
char tosend[71]; //6 for mac + 1 for "|" + 64 data 
memcpy(tosend,sendermacaddress,6);
tosend[6] = '|';
memcpy(tosend+7,datatobeprocessed,64);
webSocket.broadcastTXT(tosend, 71);

//  char *mydata1 =subStr(datatobeprocessed,"|",1);//?index starts at 1 instead of 0
//char *mydata2=subStr(datatobeprocessed,"|",2); 
// 
////long mylong;
//
//mylong = round((long)atol(mydata1));
//mylong=mylong-zerofactor;
//mylong = mylong/100; //getting down to the significant digit


//y = mx+c . value of m (calibrationfactor) calculated from plotting known values with readings. C is the raw sensor reading at the time of tare.
 // y is the sensor raw reading. x is the output for user.
 //x = (y-C)/m;
//mylong = (mylong-mytarereading)/calibrationfactor; //(y-C)/m 
//   Serial.print ("Message received.");
//
//    Serial.print("MAC= ");
//    Serial.print(sendermacaddress[3],HEX);
//    Serial.print(":");
//    Serial.print(sendermacaddress[4],HEX);
//    Serial.print(":");
//    Serial.print(sendermacaddress[5],HEX);
//    Serial.print(" Data: ");
//Serial.print(mylong); 
//Serial.print(" Battery: ");
//Serial.println(atoi(mydata2));
  //Once done processing - reset the array - can run into problems if data is arriving faster than it can be handled. Need to implement some sort of data queue
  //thought about implementing a ringbuffer with index system but - one thread - not asynchronous so shouldn't matter.
  memset(mydata, 0, sizeof(mydata));
  memset(macaddr, 0, sizeof(macaddr));
  //set datavailable to false;
  dataavailable = false;
//Push the data via websocket
//Save calibrationfactor and zerofactor for each macaddress in a file
//At boot time - open calibration and zerofactor files - and load these values into variables.  
//Tare value - raw scale reading for each device needs to saved with macaddress
//Calibration table, Name table (macaddr with name of the scale), Tare reading table, data table.
// Will need to create and associative array of currentscalerawreadings with mac addresses - which keeps getting updated as data is received. Also need to keep track of time

  
  }

void setscaletare(char *scalemacaddr, char *currentscalerawreading){
  
  }



void setup()
{ 


  Serial.begin(115200);
  delay(10);
  //Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  for(uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %d...\r\n", t);
    Serial.flush();
    delay(1000);
  }

  Serial.print("Configuring access point...");
  WiFi.softAP(ssid, password,1);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
 Serial.println(myIP);
Serial.printf("MAC address = %s\n", WiFi.softAPmacAddress().c_str());

  // Initializing the ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Problem during ESP-NOW init");
    return;
  }
  
  //esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  // We can register the receiver callback function
  esp_now_register_recv_cb(onDataReceiver);
  
  
  server.on("/", handleRoot);
  //server.onNotFound(handleNotFound);
  server.onNotFound([]() {                              // If the client requests any URI
    if (!handleFileRead(server.uri()))                  // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.begin();
 //tcpserver.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);




  if (SPIFFS.begin()){Serial.println("file system mounted");};

  //Open the "Save.txt" file and check if we were saving before the reset happened
  File q = SPIFFS.open("/Save.txt", "r");
  if (q.find("Y")){saveData=true;}
  q.close();
  
  
  
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


void loop()
{
  webSocket.loop();
  server.handleClient();
if (dataavailable){handledatafromnodes(macaddr,mydata);}

}
