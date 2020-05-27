// Very simple example sketch for Sonoff switch as described in article "Bastelfreundlich". This sketch is just for learning purposes.
// For productive use (with MQTT and advanced features) we recommend SonoffSocket.ino in this repository.

//#define REGELBETRIEB 1

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
//needed for wifimanager
#include <DNSServer.h>
//#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <stdio.h>
#include <string.h>
#include "FS.h"


extern "C" {
  #include "user_interface.h"
}

#include "wifi_security.h"

//To use MQTT, install Library "PubSubClient" and switch next line to 1
#define USE_MQTT 1

#if USE_MQTT == 1
  #include <PubSubClient.h>
  //Your MQTT Broker
  char mqtt_in_topic[1024];
  char mqtt_out_topic[1024];
  
#endif


ESP8266WebServer* server;

int gpio13Led = 13;
int gpio12Relay = 12;
int gpio0Switch = 0;

bool lamp = 0;
bool relais = 0;

unsigned long previousMillis = 0;

WiFiUDP ntpUDP;

#if USE_MQTT == 1
  WiFiClient espClient;
  PubSubClient client(espClient);
  bool status_mqtt = 1;
#endif


bool startWPSPBC() {
// from https://gist.github.com/copa2/fcc718c6549721c210d614a325271389
// wpstest.ino
  Serial.println("WPS config start");
  bool wpsSuccess = WiFi.beginWPSConfig();
  if(wpsSuccess) {
      // Well this means not always success :-/ in case of a timeout we have an empty ssid
      String newSSID = WiFi.SSID();
      if(newSSID.length() > 0) {
        // WPSConfig has already connected in STA mode successfully to the new station. 
        Serial.printf("WPS finished. Connected successfull to SSID '%s'\n", newSSID.c_str());
      } else {
        wpsSuccess = false;
      }
  }
  return wpsSuccess; 
}

void doSwitchOn()
{
    relais = 1;
    digitalWrite(gpio13Led, LOW);
    digitalWrite(gpio12Relay, relais);
    delay(1000);
}

void doSwitchOff()
{
    relais = 0;
    digitalWrite(gpio13Led, HIGH);
    digitalWrite(gpio12Relay, relais);
    delay(1000); 
}

void toggle()
{
    if(relais == 0){
      server->sendHeader("Location", String("/ein"), true);
    }else{
      server->sendHeader("Location", String("/aus"), true);
    }
}

const char CONFIG_HTML[] =
"<!DOCTYPE HTML>"
"<html>"
"<head>"
"<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
"<title>Socket Configuration</title>"
"</head>"
"<body>"
"<h1>Socket Configuration</h1>"
"<FORM action=\"/config\" method=\"post\">"
"<P>"
"SWITCH<br>"
//"<INPUT type=\"radio\" name=\"SWITCH\" value=\"1\">On<BR>"
//"<INPUT type=\"radio\" name=\"SWITCH\" value=\"0\">Off<BR>"
"<INPUT name=\"in_topic\" value=\"\">Topic (empfangen)<BR>"
"<INPUT name=\"out_topic\" value=\"\">Topic (senden)<BR>"
"<INPUT type=\"submit\" value=\"Send\"> <INPUT type=\"reset\">"
"</P>"
"</FORM>"
"</body>"
"</html>";

void handleConfig()
{
  if ((server->hasArg("in_topic"))&&(server->hasArg("out_topic"))) {
    handleSubmit();
    server->send(200, "text/html", CONFIG_HTML);
  }
  else {
    server->send(200, "text/html", CONFIG_HTML);
  }
}

void handleSubmit()
{
  String invalue;
  String outvalue;

  if (!server->hasArg("in_topic")) return returnFail("BAD ARGS");
  if (!server->hasArg("out_topic")) return returnFail("BAD ARGS");
  invalue = server->arg("in_topic");
  outvalue = server->arg("out_topic");

  // open file for writing
  File f = SPIFFS.open("/in_topic.txt", "w");
  if (!f) {
      Serial.println("file /in_topic.txt open failed");
  }
  Serial.println("====== Writing to SPIFFS file =========");
  f.println(invalue.c_str());
  f.close();

  f = SPIFFS.open("/out_topic.txt", "w");
  if (!f) {
      Serial.println("file /out_topic.txt open failed");
  }
  Serial.println("====== Writing to SPIFFS file =========");
  f.println(outvalue.c_str());
  f.close();
  snprintf(mqtt_out_topic,1023,"%s/%s",hostname,outvalue.c_str());
  snprintf(mqtt_in_topic,1023,"%s/%s",hostname,invalue.c_str());
#if USE_MQTT == 1
  client.disconnect();
#endif
}

void returnFail(String msg)
{
  server->sendHeader("Connection", "close");
  server->sendHeader("Access-Control-Allow-Origin", "*");
  server->send(500, "text/plain", msg + "\r\n");
}

void setup(void){
  Serial.begin(115200); 
  delay(5000);
  Serial.println("");
  SPIFFS.begin();
/*  // open file for writing
  File f = SPIFFS.open("/in_topic.txt", "w");
  if (!f) {
      Serial.println("file /in_topic.txt open failed");
  }
  Serial.println("====== Writing to SPIFFS file =========");
  f.println("a");
  f.close();

  f = SPIFFS.open("/out_topic.txt", "w");
  if (!f) {
      Serial.println("file /out_topic.txt open failed");
  }
  Serial.println("====== Writing to SPIFFS file =========");
  f.println("b");
  f.close();
*///  pinMode(D0, WAKEUP_PULLUP);

//  WiFi.hostname(hostname);
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
#ifdef REGELBETRIEB
  WiFi.begin(WiFi.SSID().c_str(),WiFi.psk().c_str()); // reading data from EPROM, (last saved credentials)
#else
  WiFi.begin(wifi_ssid,wifi_pwd); // reading data from EPROM, (last saved credentials)
#endif
 wifi_station_set_hostname(hostname);
  pinMode(gpio13Led, OUTPUT);
  digitalWrite(gpio13Led, lamp);
  
  pinMode(gpio12Relay, OUTPUT);
  digitalWrite(gpio12Relay, relais);

  pinMode(gpio0Switch,INPUT_PULLUP);  // Push Button for GPIO0 active LOW


  // Wait for WiFi
  Serial.println("");
  Serial.print("Verbindungsaufbau mit:  ");
  Serial.println(WiFi.SSID().c_str());

  while (WiFi.status() == WL_DISCONNECTED) {          // last saved credentials
    delay(500);
    if(lamp == 0){
       digitalWrite(gpio13Led, 1);
       lamp = 1;
     }else{
       digitalWrite(gpio13Led, 0);
       lamp = 0;
    }
    Serial.print(".");
  }
  lamp=0;
  digitalWrite(gpio13Led, 0);

  wl_status_t status = WiFi.status();
  if(status == WL_CONNECTED) {
    Serial.printf("\nConnected successful to SSID '%s'\n", WiFi.SSID().c_str());
  } else {
    WiFi.mode(WIFI_OFF);
    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //reset settings - for testing
    //wifiManager.resetSettings();
  
    //sets timeout until configuration portal gets turned off
    //useful to make it all retry or go to sleep
    //in seconds
    digitalWrite(gpio13Led, LOW);
    wifiManager.setTimeout(180);
    
    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP_12F647"
    //and goes into a blocking loop awaiting configuration
    if(!wifiManager.autoConnect("AutoConnectAP_12F647")) 
    {
      WiFi.mode(WIFI_OFF);
      WiFi.mode(WIFI_STA);
      digitalWrite(gpio13Led, HIGH);
      // WPS button I/O setup
      Serial.printf("\nCould not connect to WiFi. state='%d'\n", status);
      Serial.println("Please press WPS button on your router, until mode is indicated.");
      Serial.println("next press the ESP module WPS button, router WPS timeout = 2 minutes");
      
      while(digitalRead(gpio0Switch) == HIGH)  // wait for WPS Button active
      {
        delay(50);
        if(lamp == 0){
           digitalWrite(gpio13Led, 1);
           lamp = 1;
         }else{
           digitalWrite(gpio13Led, 0);
           lamp = 0;
        }
        Serial.print(".");
        yield(); // do nothing, allow background work (WiFi) in while loops
      }      
      Serial.println("WPS button pressed");
      lamp=0;
      digitalWrite(gpio13Led, 0);
      
      if(!startWPSPBC()) {
         Serial.println("Failed to connect with WPS :-(");  
      } else {
        WiFi.begin(WiFi.SSID().c_str(),WiFi.psk().c_str()); // reading data from EPROM, 
        while (WiFi.status() == WL_DISCONNECTED) {          // last saved credentials
          delay(500);
          Serial.print("."); // show wait for connect to AP
        }
      }
    }
  } 

  
  Serial.println("Verbunden");
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.localIP());

#if USE_MQTT == 1  
  File fr = SPIFFS.open("/in_topic.txt", "r");
  if(fr!=NULL)
  {
    if(fr.available())
    {
    String line = fr.readStringUntil('\n');
    line.trim();
 //   Serial.println(line);
    Serial.println(line.c_str());
    snprintf(mqtt_in_topic,1023,"%s/%s",hostname,line.c_str());
    }
    fr.close();
  }
  else
  {
    Serial.println("file /in_topic.txt not found!");
    snprintf(mqtt_in_topic,1023,"%s/switch/set",hostname);
  }
  fr = SPIFFS.open("/out_topic.txt", "r");
  if(fr!=NULL)
  {
    if(fr.available())
    {
    String line = fr.readStringUntil('\n');
    line.trim();
 //   Serial.println(line);
    Serial.println(line.c_str());
    snprintf(mqtt_out_topic,1023,"%s/%s",hostname,line.c_str());
    }
    fr.close();
  }
  else
  {
    Serial.println("file /out_topic.txt not found!");
    snprintf(mqtt_out_topic,1023,"%s/switch/status",hostname);
  }
  Serial.println(strlen(mqtt_in_topic));
  Serial.println(strlen(mqtt_out_topic));
  Serial.println(mqtt_in_topic);
  Serial.println(mqtt_out_topic);
  client.setServer(mqtt_server, 1883);
  client.setCallback(MqttCallback);
#endif

  server=new ESP8266WebServer(80);


  server->on("/", [](){
    toggle();
    server->send ( 302, "text/plain", "");  
  });
  
  server->on("/ein", [](){
    server->send(200, "text/html",String("")+"<h1>Schaltsteckdose ausschalten</h1><p><a href=\"aus\">AUS</a></p>");
    doSwitchOn();
  });
  
  server->on("/aus", [](){
    server->send(200, "text/html", String("")+"<h1>Schaltsteckdose einschalten</h1><p><a href=\"ein\">EIN</a></p>");
    doSwitchOff();
  });

  server->on("/toggle", [](){
    server->send(200, "text/html", String("")+"<h1>Schaltsteckdose schalten</h1><p><a href=\"toggle\">WECHSELN</a></p>");
    if(relais == 0){
      doSwitchOn();
    }
    else
    {
      doSwitchOff();
    }
  });
  server->on("/config", handleConfig);
  server->on("/topics", [](){
    server->send(200, "text/html",String(mqtt_in_topic)+" "+String(mqtt_out_topic));
    doSwitchOn();
  });
  
 
  digitalWrite(gpio13Led, HIGH);
  delay(300);
  server->begin();
  Serial.println("HTTP server started");
  
    
/*      pinMode(BUILTIN_LED, OUTPUT);
  // Connect D0 to RST to wake up
  pinMode(D0, WAKEUP_PULLUP);
  // LED: LOW = on, HIGH = off
  Serial.println("Start blinking");
  for (int i = 0; i < 20; i++) {
    digitalWrite(BUILTIN_LED, LOW);
    delay(100);
    digitalWrite(BUILTIN_LED, HIGH);
    delay(100);
  }
  Serial.println("Stop blinking");
  Serial.printf("Sleep for %d seconds\n\n", 5);
  // convert to microseconds
  ESP.deepSleep(5 * 1000000);
  Serial.println("sleep finished");
*/
/*    Serial.print("Going into deep sleep - waking up at: ");
    printTime(massaged, tcr -> abbrev, "wake up");
    Serial.println(massaged-local);
    ESP.deepSleep((massaged-local)*1e06); // 20e6 is 20 microseconds
*/}
void loop(void)
{
  if(digitalRead(gpio0Switch) == LOW)

  {
    Serial.println("Button pressed!");
    if(relais == 0){
      doSwitchOn();
    }else{
      doSwitchOff();
    }

  }
  //Webserver 
  server->handleClient();
  #if USE_MQTT == 1
//MQTT
   if (!client.connected()) {
    MqttReconnect();
   }
   if (client.connected()) {
    MqttStatePublish();
   }
  client.loop();
#endif  

} 
#if USE_MQTT == 1
void MqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Received message [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.print("Received payload [");
    Serial.print((char *)payload);
    Serial.print("] ");
//    if(strcmp(topic,mqtt_in_topic)==0){
      // Switch on
      if ((char)payload[0] == '1') {
        doSwitchOn();
      //Switch off
      } else {
        doSwitchOff();
      }
 //   }
}

void MqttReconnect() {
  String clientID = "SonoffSocket_"; // 13 chars
  clientID += WiFi.localIP().toString();//17 chars
  int count=5;

  if ((!client.connected())&&(--count>0)) {
    Serial.print("Connect to MQTT-Broker");
    if (client.connect(clientID.c_str(),mqtt_user,mqtt_pass)) {
      Serial.print("connected as clientID:");
      Serial.println(clientID);
      //publish ready
      client.publish(mqtt_out_topic, "mqtt client ready");
      //subscribe in topic
    } else {
      Serial.print("failed: ");
      Serial.print(client.state());
      Serial.println(" try again...");
      delay(2000);
    }
  }
  if(client.connected())
  {
      Serial.println("subscribing to ");
      Serial.println(mqtt_in_topic);
      client.subscribe(mqtt_in_topic);
  }
}

void MqttStatePublish() {
  if (relais == 1 and not status_mqtt)
     {
      status_mqtt = relais;
      client.publish(mqtt_out_topic, "on");
      Serial.print("MQTT publish - topic: ");
      Serial.print(mqtt_out_topic);
      Serial.println(": on");
     }
  if (relais == 0 and status_mqtt)
     {
      status_mqtt = relais;
      client.publish(mqtt_out_topic, "off");
      Serial.println("MQTT publish: off");
     }
}

#endif

