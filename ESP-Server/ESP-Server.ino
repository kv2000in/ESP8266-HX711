/*
 * ESP-01 Formfactor - 1 blue LED
19:33:54.810 -> Flash real id:   00144020
19:33:54.810 -> Flash real size: 1048576 bytes
19:33:54.810 -> 
19:33:54.810 -> Flash ide  size: 1048576 bytes
19:33:54.810 -> Flash ide speed: 40000000 Hz
19:33:54.810 -> Flash ide mode:  DIO
19:33:54.846 -> Flash Chip configuration ok

ESP-12 Formfctor
19:39:53.826 -> Flash real id:   001640E0
19:39:53.826 -> Flash real size: 4194304 bytes
19:39:53.826 -> 
19:39:53.826 -> Flash ide  size: 4194304 bytes
19:39:53.826 -> Flash ide speed: 40000000 Hz
19:39:53.827 -> Flash ide mode:  DIO
19:39:53.827 -> Flash Chip configuration ok.
19:39:53.859 -> 


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
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
//#include "FS.h" //LittleFS
#include "LittleFS.h"
#include <ArduinoOTA.h>
#include <string.h>
#include <espnow.h>
//#include <SPIFFS.h>
//#include <SPIFFSEditor.h>

char mydata[8]; 
char macaddr[6];
float myfloatbytes;
int myintbytes;
//long zerofactor=-817214; //not needed 
//int calibrationfactor=1;
int mytarereading=0;
bool dataavailable = false;


void onDataReceiver(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(mydata,incomingData,8);
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


AsyncWebServer server(80);
AsyncWebSocket webSocket("/ws"); 

unsigned long currESPSecs, currTime,timestamp, zeroTime;

void handlecommandsfromwebsocketclients(char * datasentbywebsocketclient){
  //Commands will include the MAC Address - Tare or MAC Address - save common name or MAC Address 
  }

//flag to use from web update to reboot the ESP
bool shouldReboot = false;

void onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
  //Handle body
}
File p;
void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  //Handle upload
   
    if(!index){
      Serial.printf("UploadStart: %s\n", filename.c_str());
       p = LittleFS.open(filename, "w");
    }
      Serial.printf("UploadStart: (%u), (%u)\n", index, len);
   p.seek(index, SeekSet);
   Serial.printf("File position = %u",p.position());
   for(size_t i=0; i<len; i++){
   p.write(data[i]);
   
    
  }
      
    if(final)
      {
        Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index+len);
      request->send (200, "text/plain", String (index+len));
      p.close();
      }
}



void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    //client connected
    os_printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Z#a020a60a86ce|-811493|2022-4-12 21:33:49");
    client->printf("C#a020a60a86ce|500/285|2022-4-12 21:33:49"); 
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    //client disconnected
    os_printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    //error was received from the other end
    os_printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    //pong message was received (in response to a ping request maybe)
    os_printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    //data packet
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      os_printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);
      if(info->opcode == WS_TEXT){
        data[len] = 0;
        os_printf("%s\n", (char*)data);
        //handlecommandsfromwebsocketclients(data);
      } else {
        for(size_t i=0; i < info->len; i++){
          os_printf("%02x ", data[i]);
        }
        os_printf("\n");
      }
      if(info->opcode == WS_TEXT)
        client->text("I got your text message");
      else
        client->binary("I got your binary message");
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0)
          os_printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        os_printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      os_printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);
      if(info->message_opcode == WS_TEXT){
        data[len] = 0;
        os_printf("%s\n", (char*)data);
      } else {
        for(size_t i=0; i < len; i++){
          os_printf("%02x ", data[i]);
        }
        os_printf("\n");
      }

      if((info->index + len) == info->len){
        os_printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          os_printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if(info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
}



int fractionoccupiedspaceonfilesystem(){
FSInfo fs_info;
LittleFS.info(fs_info);
return fs_info.usedBytes;
//fs_info.totalBytes 
  
  }

void handledatafromnodes(char *sendermacaddress,char *datatobeprocessed){
  //Data arrives as 4bytes of float -scale reading and 2 bytes of integer - batteryvoltage
//Send data as is to websocket
char tosend[16]; //6 for mac + 8 for data(really just 6 for data - 4 for float and 2 for int+2 extra) + 2 extra
memcpy(tosend,sendermacaddress,6);
memcpy(tosend+6,datatobeprocessed,8);
webSocket.binaryAll(tosend, 16);
convertrawdata(datatobeprocessed);
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

void convertrawdata(char * rawdata){
  //Thinking about saving everything binary - as it will save space. All binary to ascii conversions to be done by javascript on the browsers.
  //So this function is here just for testing.
 memcpy(&myfloatbytes,rawdata,4);
 memcpy(&myintbytes,rawdata+4,2);
Serial.print("Reading = ");
Serial.print(myfloatbytes);
Serial.print(" BatteryVoltage= ");
Serial.println(myintbytes);
 
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
WiFi.mode(WIFI_STA);
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
  
  


      

//    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
//        //request->send(200, "text/html", INDEX_HTML);
//   request->send(200, "text/html", "HELLO WORLD");
//    
//    });





      // respond to GET requests on URL /heap
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.on("/upload", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.printf("%u \n",fractionoccupiedspaceonfilesystem());
    request->send(200, "text/html", "<p>Available space="+String (fractionoccupiedspaceonfilesystem())+"</p><br><br><br><form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>");
  });
  // upload a file to /upload
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){
    
  }, onUpload);

  // send a file when /index is requested
  server.on("/index", HTTP_GET, [](AsyncWebServerRequest *request){
   AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/index.html");
//response->addHeader("Last","ESP Async Web Server");
//
response->addHeader("ETag","33a64df551425fcc55e4d42a148795d9f25f89d4");
response->addHeader("Last-Modified","Mon, 17 Apr 2022 14:00:00 GMT");
request->send(response); 
   //
  });


  // Simple Firmware Update Form
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>");
  });
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){
    shouldReboot = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot?"OK":"FAIL");
    response->addHeader("Connection", "close");
    request->send(response);
  },[](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index){
      Serial.printf("Update Start: %s\n", filename.c_str());
      Update.runAsync(true);
      if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)){
        Update.printError(Serial);
      }
    }
    if(!Update.hasError()){
      if(Update.write(data, len) != len){
        Update.printError(Serial);
      }
    }
    if(final){
      if(Update.end(true)){
        Serial.printf("Update Success: %uB\n", index+len);
      } else {
        Update.printError(Serial);
      }
    }
  });

  // attach filesystem root at URL /fs
  //server.serveStatic("/fs", LittleFS, "/data/");
  //server.serveStatic("/", LittleFS,"/data/index.html").setLastModified("Mon, 17 Apr 2022 14:00:00 GMT");

//AsyncStaticWebHandler* handler = &server.serveStatic("/", LittleFS, "/index.html").setLastModified("Mon, 17 Apr 2022 14:00:00 GMT");
server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
server.serveStatic("/", LittleFS, "/").setLastModified("Mon, 17 Apr 2022 14:00:00 GMT");
//server.serveStatic("/", LittleFS, "index.html").setLastModified("Mon, 17 Apr 2022 14:00:00 GMT");

server.onNotFound([](AsyncWebServerRequest *request){
    Serial.printf("NOT_FOUND: ");
    if(request->method() == HTTP_GET)
      Serial.printf("GET");
    else if(request->method() == HTTP_POST)
      Serial.printf("POST");
    else if(request->method() == HTTP_DELETE)
      Serial.printf("DELETE");
    else if(request->method() == HTTP_PUT)
      Serial.printf("PUT");
    else if(request->method() == HTTP_PATCH)
      Serial.printf("PATCH");
    else if(request->method() == HTTP_HEAD)
      Serial.printf("HEAD");
    else if(request->method() == HTTP_OPTIONS)
      Serial.printf("OPTIONS");
    else
      Serial.printf("UNKNOWN");
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if(request->contentLength()){
      Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }

    int headers = request->headers();
    int i;
    for(i=0;i<headers;i++){
      AsyncWebHeader* h = request->getHeader(i);
      Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

    int params = request->params();
    for(i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isFile()){
        Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if(p->isPost()){
        Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404);
  });


  
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    if(!index)
      Serial.printf("BodyStart: %u\n", total);
    Serial.printf("%s", (const char*)data);
    if(index + len == total)
      Serial.printf("BodyEnd: %u\n", total);
  });

 webSocket.onEvent(onEvent);
  server.addHandler(&webSocket);
  server.begin();
  //server.onNotFound(notFound);





  if (LittleFS.begin()){Serial.println("file system mounted");};


  
  
  
  /* ************OTA-Update********************* */

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
  if(shouldReboot){
    Serial.println("Rebooting...");
    delay(100);
    ESP.restart();
  }
 
  ArduinoOTA.handle();
if (dataavailable){handledatafromnodes(macaddr,mydata);}
webSocket.cleanupClients();
}
