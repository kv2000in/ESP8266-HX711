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

#include <string.h>

WiFiClient client;

// WiFi credentials.
const char* WIFI_SSID = "ESP";
const char* WIFI_PASS = "monte123";
const char* host = "192.168.4.1";  // TCP Server IP
const int   port = 9999;            // TCP Server Port

int batteryVoltage;   
int R1=22;//22k
int R2=10;//10k

String mystring;


// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 4; //Hardwired
const int LOADCELL_SCK_PIN = 5; //Hardwired
//const float calibrationFactor=226.626;
const float calibrationFactor=1;
unsigned long previousMillis = 0;     
long timeoutinterval = 1500; 


HX711 scale;




void connect() {


WiFi.mode(WIFI_STA);
WiFi.begin(WIFI_SSID, WIFI_PASS);
//WiFi.config(IPAddress(192,168,1,204), IPAddress(192,168,1,2), IPAddress(255,255,255,0),IPAddress(192,168,1,2));
unsigned long wifiConnectStart = millis();

while (WiFi.status() != WL_CONNECTED) {
// Check to see if
if (WiFi.status() == WL_CONNECT_FAILED) {
Serial.println("WiFi Connection Failed");
delay(10000);
}

delay(500);

// Only try for 5 seconds.
if (millis() - wifiConnectStart > 15000) {
Serial.println("WiFi Stopped Trying");
return;
}

}


Serial.println("WiFi Connected");
}
void senddata(){

if (!client.connect(host, port)) {

return;
}
char str[32];
//ADC*(1.1/1024) will give the Vout at the voltage divider
//V=(Vout*((R1+R2)/R2))*1000 miliVolts
batteryVoltage = ((analogRead(A0)*(1.1/1024))*((R1+R2)/R2))*1000;
// convert the time into a distance

if (scale.is_ready()) {


float reading = scale.get_units(10); 

Serial.println(reading);

dtostrf(reading,10, 4,str);
//webSocket.broadcastTXT(datastr, strlen(datastr));
}

int alength = strlen(str);
str[alength]=':';
//Add the vcc value after ':'
itoa( batteryVoltage, str+alength+1, 10 ); // for some reason +1 outputs starnge 2:32:283:26 or  2:11305:275:26



Serial.println(str);
client.print(str);

Serial.println("sent");

delay(5);
//Close the socket - server is closing after one receive at the moment so it may not be necessary to close by the client
client.stop();
// Serial.println(millis()-currentmillis); //=1489
}










void setup()   {
Serial.begin(9600);
scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
//scale.set_scale(211.8*10.7);                      // this value is obtained by calibrating the scale with known weights; see above and the https://github.com/bogde/HX711 README for details
scale.power_up();
delay(20);
scale.set_scale(226.626); 
//scale.tare(); if tare after eac wake up - will always output zero

Serial.println("All good in setup");
connect();
delay(100);
senddata();
delay(50); 
scale.power_down();
delay(20);

ESP.deepSleep(30e6);
}


void loop() {


}
