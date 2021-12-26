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

void onDataReceiver(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
 
   Serial.print ("Message received.");

    
Serial.print("char: ");
for (int k = 0; k <= 64; k++) {
   
 
   Serial.print((char)incomingData[k]);
    Serial.print(" ");
}
Serial.println(); 
  
}

#define MAX_STRING_LEN  32



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
WiFiServer tcpserver(9999);//TCPserver
WebSocketsServer webSocket = WebSocketsServer(81);

boolean saveData = false;
boolean broadcast = false;


unsigned long currESPSecs, currTime,timestamp, zeroTime;

static const char PROGMEM INDEX_HTML[] = R"rawliteral(
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
 tcpserver.begin();
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


}
