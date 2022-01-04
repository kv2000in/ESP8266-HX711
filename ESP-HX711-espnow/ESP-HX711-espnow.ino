/**
 * ESP-NOW
 * 
 * Wireshark each send to a defined peer results in 6 mbps => 2 packets, 2 mbps =>2 packets, 1 mbps => ?6 packets. 
 * All packets with same unique 4 digit random Sequence number. All except 1st one has Retransmission Flag set.
 * By the time last ACK is received by MAC layer = total 12 mSec
 * However - if the only peer = FF:FF:FF:FF:FF:FF - only one packet sent @ 1 mbps - No retransmissions.
 * What if I added a specific peer and a broadcast peer? - Both will have retransmitted packets?
 * specific peer gets retransmission as above with same Seq N - and broadcast only gets one packet sent with the +1 SeqN.
 * So - still worth it to leave the broadcast address as a peer 
  
*/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include "HX711.h"
// Mac address of the slave
uint8_t peer0[] = {0x86, 0xCC, 0xA8, 0xAA, 0x20, 0xF9};//86:CC:A8:AA:20:F9 softAP MAC of server
uint8_t macAddr[6];
int batteryVoltage; //Can either use ADC or ESP.getVcc(). ADC has to be floating for getVcc to work. 

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 4; //Hardwired
const int LOADCELL_SCK_PIN = 5; //Hardwired
     
char str[64];
int resendcounter;
int maxresendattempts=10;
bool sendsuccess;
unsigned long myTime;
HX711 scale;


int R1=22;//22k
int R2=10;//10k

bool DEBUG = false; //espnow call back function was wrapped inside debug - which was causing reset


void onSent(uint8_t *mac_addr, uint8_t sendStatus) {

if (sendStatus==0)
      {
      resendcounter=0;
      sendsuccess = true;
      }
else {
        if (resendcounter<maxresendattempts) 
        {
          senddata();
        } 
        else 
        {
    
          if (DEBUG){Serial.println("ESP-NOW send failed");}
        }

      }

}





void createdata(){
scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
//scale.set_scale(211.8*10.7);                      // this value is obtained by calibrating the scale with known weights; see above and the https://github.com/bogde/HX711 README for details
scale.power_up();
delay(5);
  str[64]={};
WiFi.mode(WIFI_STA);
//WiFi.macAddress(macAddr);
//memcpy(str,macAddr,6);
//str[6]='|';
//ADC*(1.1/1024) will give the Vout at the voltage divider
//V=(Vout*((R1+R2)/R2))*1000 miliVolts
batteryVoltage = ((analogRead(A0)*(1.1/1024))*((R1+R2)/R2))*1000;

float reading; 
//scale.set_scale(226.626); 
scale.set_scale(1);
do {delayMicroseconds(10);}
while (not (scale.is_ready()));

if (DEBUG) {Serial.print("After Scale is Ready"); myTime = millis(); Serial.println(myTime); }//130 mSec

reading = scale.get_units(10); 


if (DEBUG) {Serial.println(reading);}


//dtostrf(reading,12, 2,str+7);
dtostrf(reading,12, 2,str);
//ltoa(reading,str,12);
//webSocket.broadcastTXT(datastr, strlen(datastr));
int alength = strlen(str);
str[alength]='|';

//Add the battery value after ':'
itoa( batteryVoltage, str+alength+1, 10 ); // for some reason +1 outputs starnge 2:32:283:26 or  2:11305:275:26
alength = strlen(str);
str[alength]='\0';

if (DEBUG) {Serial.println(str);}
scale.power_down();
delay(5);
  if (DEBUG) {Serial.print("CreateDataend"); myTime = millis(); Serial.println(myTime);} //1006 mSec
  }
void prepareESPNOW() {
WiFi.mode(WIFI_STA);
WiFi.persistent( false );
resendcounter=0;
    // Initializing the ESP-NOW
  if (esp_now_init() != 0) {
    if (DEBUG){Serial.println("Problem during ESP-NOW init");}
    return;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  // Register the peer
  //Serial.println("Registering a peer");
  esp_now_add_peer(peer0, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
 
  if(DEBUG){ Serial.println("Registering send callback function");}
  esp_now_register_send_cb(onSent);

}
 
 void senddata(){
 esp_now_send(NULL, (uint8_t *) str, 64);
 resendcounter++;
}










void setup()   {
resendcounter=0;
sendsuccess = false;

if (DEBUG){Serial.begin(115200);}
if (DEBUG){Serial.println("Running Setup");}

if (DEBUG) {Serial.print("AfterSetup"); myTime = millis(); Serial.println(myTime);} //62 mSec


createdata();

prepareESPNOW();

if (DEBUG) {Serial.print("After prepareESPNow"); myTime = millis(); Serial.println(myTime);}//1007 mSec 
//while ((sendsuccess==false)&&(resendCounter<maxresendattempts)){
senddata();
//resendCounter++;
delayMicroseconds(2000);
//}

if (DEBUG) {Serial.println("sleep time");}
ESP.deepSleep(30e6);
}


void loop() {


}
