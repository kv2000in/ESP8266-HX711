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
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "FS.h"
#include <ArduinoOTA.h>
#include <string.h>
#include <espnow.h>

char mydata[8]; 
char macaddr[6];
float myfloatbytes;
int myintbytes;
long zerofactor=-817214; //not needed 
int calibrationfactor=1;
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

                td{
                    border: groove;
                }
                .td-hidden{
                    display: none;
                }
                .td-actions{
                    visibility: visible;
                }
      </style>
      <script>
                //Will be obsolete once the code for requesting these values from the server are written
                 var calibrationfactor;
                    var    zerorawvalue;
        var websock;
        var boolClientactive;
        var boolHasZeroBeenDone = false;
        
        function start() {
          websock = new WebSocket('ws://' + window.location.hostname + '/ws');
                    websock.binaryType = "arraybuffer"; //need to do this when dealing with binary data.
          websock.onopen = function(evt) { console.log('websock open');
            
            
            
          };
          websock.onclose = function(evt) { console.log('websock close'); };
          websock.onerror = function(evt) { console.log(evt); };
          websock.onmessage = function(evt) {
            console.log(evt);
                           if(evt.data instanceof ArrayBuffer) {
                                // binary frame
                    //const view = new DataView(evt.data);
                    //console.log(view.getInt32(0));
                    
                        handleserverbinarydata(evt.data);
                           } else {
                    // text frame
                        console.log(evt.data);
                    }

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
                
            function handleserverbinarydata(serverbinarydata){
                //16 bytes of data = 6 for mac + 8 for data(really just 6 for data - 4 for float and 2 for int+2 extra) + 2 extra
                var mybinarydataarray= new Uint8Array(serverbinarydata);
                var mysendermacaddress = mybinarydataarray.slice(0,6);
                var myscalefloatvaluebytes = serverbinarydata.slice(6,10); 
                var mybatteryvoltagebytes = serverbinarydata.slice(10,12);
                var myfloatview = new DataView(myscalefloatvaluebytes);
                var myintview = new DataView(mybatteryvoltagebytes);
                var myscalefloatvalue = myfloatview.getFloat32(0,true); // Signed 32 bit float, little endian. https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/getFloat32
                var mybatteryvoltage = myintview.getInt16(0,true);
               //Generate timestamp for this data
                var today = new Date();
                var date = today.getFullYear()+'-'+(today.getMonth()+1)+'-'+today.getDate();
                var time = today.getHours() + ":" + today.getMinutes() + ":" + today.getSeconds();
                var dateTime = date+' '+time;
            //call the data handler routine
                handlecleanedupdata(buf2hex(mysendermacaddress),myscalefloatvalue,mybatteryvoltage,dateTime);
            
            }
      //https://stackoverflow.com/questions/40031688/javascript-arraybuffer-to-hex
                
            function buf2hex(buffer) { // buffer is an ArrayBuffer
  return [...new Uint8Array(buffer)]
      .map(x => x.toString(16).padStart(2, '0'))
      .join('');
}
      
                function handlecleanedupdata(macaddr,rawreading,battery,mytime){
                var mydatalist = document.getElementById("availablesensors");
                  var n;
                  var alreadypresent="false";
                  for (n=0;n<mydatalist.options.length;n++)
                      {
                          if (macaddr == mydatalist.options[n].value){
                          //Don't add if the macaddr already in the list
                          alreadypresent="true"
                        // Table must already have been expanded so just fill up the data
                              //Fill up the columns with data
                      document.getElementById("addr"+macaddr).innerHTML=macaddr;
                      //User assignment of names - may be store it on the server
                      document.getElementById("rawreading"+macaddr).innerHTML=rawreading;
                    document.getElementById("reading"+macaddr).innerHTML=Math.round(((rawreading-zerorawvalue)/100)*calibrationfactor);
                      document.getElementById("battery"+macaddr).innerHTML=battery;
                      document.getElementById("time"+macaddr).innerHTML=mytime;
                              
                              
                              break;
                          }

                      }
                    // Data being received from a MAC address which isn't alreadypresent in the list
                  if (alreadypresent=="false"){
                      
                      //Add this MAC address to the list of all online MAC addresses 
                      const newele0=document.createElement("Option");
                        newele0.value=macaddr;
                        mydatalist.appendChild(newele0);
                      //Request stored calibration factor and zero level value from the server
                        calibrationfactor = 500/285; //Actual weight/Displayed weight when calibration factor = 1.
                        zerorawvalue = -811493;
                      //Expand the sensortable to add another row with children whose id is assigned by concatenating macaddress
                        const newele1=document.createElement("tr");
                        newele1.setAttribute("id",macaddr);
                        var mytable = document.getElementById("sensortable");
                        mytable.appendChild(newele1);
                        var myrow = document.getElementById(macaddr);
                      //Add columns to the new row
                        const newele2 = document.createElement("td");
                        newele2.setAttribute("id","addr"+macaddr);
                        newele2.setAttribute("class","td-hidden");
                        const newele2a = document.createElement("td");
                        newele2a.setAttribute("id","name"+macaddr);
                        const newele3 = document.createElement("td");
                        newele3.setAttribute("id","rawreading"+macaddr);
                        newele3.setAttribute("class","td-hidden");
                        const newele3a = document.createElement("td");
                        newele3a.setAttribute("id","calibrationfactor"+macaddr);
                        newele3a.setAttribute("class","td-hidden");
                        const newele3b = document.createElement("td");
                        newele3b.setAttribute("id","reading"+macaddr);
                        const newele4 = document.createElement("td");
                        newele4.setAttribute("id","battery"+macaddr);
                        const newele5 = document.createElement("td");
                        newele5.setAttribute("id","time"+macaddr);
                        const newele6 = document.createElement("td");
                        newele6.setAttribute("id","actions"+macaddr);
                        newele6.setAttribute("class","td-actions");
                        myrow.appendChild(newele2);
                        myrow.appendChild(newele2a);
                        myrow.appendChild(newele3);
                        myrow.appendChild(newele3a);
                        myrow.appendChild(newele3b);
                        myrow.appendChild(newele4);
                        myrow.appendChild(newele5);
                        myrow.appendChild(newele6);
                      //Fill up the columns with data
                      document.getElementById("addr"+macaddr).innerHTML=macaddr;
                      //User assignment of names - may be store it on the server
                      document.getElementById("rawreading"+macaddr).innerHTML=rawreading;
                       document.getElementById("reading"+macaddr).innerHTML=Math.round(((rawreading-zerorawvalue)/100)*calibrationfactor);
                      document.getElementById("battery"+macaddr).innerHTML=battery;
                      document.getElementById("time"+macaddr).innerHTML=mytime;
                      //Actions Zero/Tare, Calibrate
                      const newele7 = document.createElement("button");
                      newele7.setAttribute("id","zero"+macaddr);
                      newele7.innerHTML="Zero";
                      const newele8 = document.createElement("button");
                      newele8.setAttribute("id","calibrate"+macaddr);
                      newele8.innerHTML="Calibrate";
                      document.getElementById("actions"+macaddr).appendChild(newele7);
                      document.getElementById("actions"+macaddr).appendChild(newele8);
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
    <label>Available Scales</label>
        <input list="availablesensors">
    
    <a href="/Data.txt">Download Data</a>
    <br><br>
    <a href="/Zero.txt">Download Zero File</a>
    <br><br>
    <button id = "btCLOSEWS" onclick="websock.close();">Close Websocket</button>
    <br>
        <table id="sensortable">
        <tr id = "tableheadings">
            <td class = "td-hidden">MacAddr</td>
            <td>Name</td>
            <td class = "td-hidden">RawReading</td>
            <td class = "td-hidden">CalibrationFactor</td>
            <td>Reading</td>
            <td>Battery</td>
            <td>Timestamp</td>
            <td class = "td-actions">Actions</td>
            </tr>
        </table>
        
        <datalist id="availablesensors">
</datalist>
        
  </body>
  
</html>

)rawliteral";




//void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
//{
//  Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
//  switch(type) {
//    case WStype_DISCONNECTED:
//      Serial.printf("[%u] Disconnected!\r\n", num);
//      break;
//    case WStype_CONNECTED:
//      {
//        IPAddress ip = webSocket.remoteIP(num);
//        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
//      //itoa( cm, str, 10 );
//      //  webSocket.sendTXT(num, str, strlen(str));
//      }
//      break;
//    case WStype_TEXT:
//    {
//      //Serial.printf("[%u] get Text: %s\r\n", num, payload);
//
//      //Payload will be in the form of 3 alphapets and 3 digits
//      //DATA-105 means it is Data and value is 105
//      //COMMAND-ZERO means Command ZERO
//      //TIME-12345678 means epoch timestamp in seconds
//      
//      char *split1,*split2 ;
//     char *mystring = (char *)payload;
//      split1=subStr(mystring, "-", 1);
//      split2=subStr(mystring, "-", 2);
//      
//      if (strcmp(split1,"COMMAND") == 0)
//        {
//          //Serial.println("Received Command");
//          //Serial.println(split2);
//          if (strcmp(split2,"ZERO") == 0){
//           // Serial.println("Zero command received");
////            if (!clientHeight){
////              Serial.println("No client data");
////              } else {
////            ZeroOffset=serverHeight-clientHeight;
////            zeroTime=timestamp+((millis()/1000)-currESPSecs);
////            //Serial.println(currTime);
////            File f = SPIFFS.open("/Zero.txt", "w");
////            f.print("Time:");
////            f.print(zeroTime);
////            f.print("\n");
////            f.print("ZeroOffset:");
////            f.print(ZeroOffset);
////            f.print("\n");
////            f.close();
////            //Reset the currESPsecs to current millis at the zerotime
////            //So the data save time will be zero time + secs elapsed since zero time
////            currESPSecs = millis()/1000;
////            
////            }
//          } else if (strcmp(split2,"STARTSAVE") == 0){
//            
//            if (!saveData){
//              saveData=true;
//            File f = SPIFFS.open("/Data.txt", "w");
//            f.close();
//            //Create a file which keeps log of current saving process
//            File s = SPIFFS.open("/Save.txt", "w");
//            s.print("Y");
//            s.close();
//            }
//            
//            } 
//
//            else if (strcmp(split2,"STOPSAVE") == 0) 
//            {
//              if (saveData){saveData=false;
//                File s = SPIFFS.open("/Save.txt", "w");
//                s.print("N");
//                s.close();
//              }
//              }
//
//             
//            else if (strcmp(split2,"RUSAVING") == 0) 
//            {
//              if (saveData){
//                webSocket.sendTXT(num, "Y");
//                } else {
//                  
//                webSocket.sendTXT(num, "N");  
//                  }
//              }
//
//        
//        } 
//      else if (strcmp(split1,"DATA") == 0)
//        {
//          //Serial.println("Received Data");
//          //Serial.println(split2);
//          //clientHeight=atoi(split2);
//          if (saveData){
//          //currTime=timestamp+((millis()/1000)-currESPSecs);
//          currTime=((millis()/1000)-currESPSecs);
//          File f = SPIFFS.open("/Data.txt", "a");
//          f.print(currTime);
//          f.print(":");
//          f.print("C:");
//          //f.print(clientHeight);
//          f.print("\n");
//          f.close();
//          }
//          if(broadcast){
//           webSocket.broadcastTXT(split2, strlen(split2));
//            } else {
//              //If not broadcast - send to browser client num =1
//              webSocket.sendTXT(1,split2, strlen(split2));
//              
//              }
//          
//        } 
//      else if (strcmp(split1,"TIME") == 0)
//        {
//          //Serial.println("Received TimeStamp");
//          //Serial.println(split2);
//          timestamp=atol(split2);
//          //currTime=split2;
//          currESPSecs = millis()/1000;
//          //Serial.println(timestamp);
//          //Serial.println(currESPSecs);
//        }
//      else 
//      {
//          Serial.printf("Unknown-");
//          Serial.printf("[%u] get Text: %s\r\n", num, payload);
//          // send data to all connected clients
//          //webSocket.broadcastTXT(payload, length);
//      }
//    }
//    // clientHeight=atoi((const char *)payload);
//     // Serial.println((const char *)payload);
//     // itoa( cm, str, 10 );
//      // webSocket.sendTXT(0, str, strlen(str));
//      break;
//    case WStype_BIN:
//      Serial.printf("[%u] get binary length: %u\r\n", num, length);
//      hexdump(payload, length);
//
//      // echo data back to browser
//      webSocket.sendBIN(num, payload, length);
//      break;
//    default:
//      Serial.printf("Invalid WStype [%d]\r\n", type);
//      break;
//  }
//}
void handlecommandsfromwebsocketclients(char * datasentbywebsocketclient){
  //Commands will include the MAC Address - Tare or MAC Address - save common name or MAC Address 
  }

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    //client connected
    os_printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
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
SPIFFS.info(fs_info);
return fs_info.usedBytes/fs_info.totalBytes; 
  
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


void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
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
  
  


      

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", INDEX_HTML);
     
    
    });

 webSocket.onEvent(onEvent);
  server.addHandler(&webSocket);
  server.begin();
  server.onNotFound(notFound);





  if (SPIFFS.begin()){Serial.println("file system mounted");};

  //Open the "Save.txt" file and check if we were saving before the reset happened
  File q = SPIFFS.open("/Save.txt", "r");
  if (q.find("Y")){//saveData=true;
    }
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
 
  
if (dataavailable){handledatafromnodes(macaddr,mydata);}

}
