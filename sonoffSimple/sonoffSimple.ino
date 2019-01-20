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
#include <TimeLib.h>
#include <Timezone.h>    // https://github.com/JChristensen/Timezone
#include <stdio.h>
#include <string.h>


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

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP,"2.de.pool.ntp.org");

// You can specify the time server pool and the offset, (in seconds)
// additionaly you can specify the update interval (in milliseconds).
// NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

// Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
Timezone CE(CEST, CET);

char timestrbuf[100];

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

void setup(void){
  Serial.begin(115200); 
  delay(5000);
  Serial.println("");
//  pinMode(D0, WAKEUP_PULLUP);

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
  snprintf(mqtt_in_topic,1023,"%s/switch/set",hostname);
  Serial.println(mqtt_in_topic);
  snprintf(mqtt_out_topic,1023,"%s/switch/status",hostname);
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
    timeClient.update();
    unsigned long epoch=timeClient.getEpochTime();
    TimeChangeRule *tcr;
    time_t utc;
    utc = epoch;
    time_t local=CE.toLocal(utc, &tcr);
    printTimeToBuffer(local,tcr -> abbrev);
    server->send(200, "text/html",String("")+"Schaltsteckdose ausschalten<p>"+timestrbuf+"</p><p><a href=\"aus\">AUS</a></p>");
    doSwitchOn();
  });
  
  server->on("/aus", [](){
    timeClient.update();
    unsigned long epoch=timeClient.getEpochTime();
    TimeChangeRule *tcr;
    time_t utc;
    utc = epoch;
    time_t local=CE.toLocal(utc, &tcr);
    printTimeToBuffer(local,tcr -> abbrev);
    server->send(200, "text/html", String("")+"Schaltsteckdose einschalten<p>"+timestrbuf+"</p><p><a href=\"ein\">EIN</a></p>");
    doSwitchOff();
  });
  digitalWrite(gpio13Led, HIGH);
  delay(300);
  server->begin();
  Serial.println("HTTP server started");
  
    
  timeClient.begin();
  Serial.println("NTP client started");
  timeClient.update();
  Serial.println(timeClient.getFormattedTime());
  unsigned long epoch=timeClient.getEpochTime();
  Serial.println(epoch);

    TimeChangeRule *tcr;
    time_t utc;
    utc = epoch;

    printTime(utc, "UTC", "Universal Coordinated Time");
    printTime(CE.toLocal(utc, &tcr), tcr -> abbrev, "Bratislava");
    time_t local=CE.toLocal(utc, &tcr);
    Serial.println(local);
    time_t tenmin=10*60;
    time_t massaged=(local/tenmin)*tenmin;
    printTime(massaged, tcr -> abbrev, "tenner");
    massaged=massaged+tenmin+8*60;


    printTimeToBuffer(local,tcr -> abbrev);
    Serial.println(timestrbuf);
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
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= 1000)
  {
  
  timeClient.update();

//  Serial.println(timeClient.getFormattedTime());
    previousMillis=currentMillis;
  }  

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
  Serial.println("got mqtt topic");
  Serial.println(topic);
  Serial.println((char *)payload);
  // Switch on
  if ((char)payload[0] == '1') {
    doSwitchOn();
  //Switch off
  } else {
    doSwitchOff();
  }

}

void MqttReconnect() {
  String clientID = "SonoffSocket_"; // 13 chars
  clientID += WiFi.macAddress();//17 chars

  while (!client.connected()) {
    Serial.print("Connect to MQTT-Broker");
    if (client.connect(clientID.c_str(),mqtt_user,mqtt_pass)) {
      Serial.print("connected as clientID:");
      Serial.println(clientID);
      //publish ready
      client.publish(mqtt_out_topic, "mqtt client ready");
      //subscribe in topic
      Serial.println("subscribing to ");
      Serial.println(mqtt_in_topic);
      client.subscribe(mqtt_in_topic);
    } else {
      Serial.print("failed: ");
      Serial.print(client.state());
      Serial.println(" try again...");
      delay(5000);
    }
  }
}

void MqttStatePublish() {
  if (relais == 1 and not status_mqtt)
     {
      status_mqtt = relais;
      client.publish(mqtt_out_topic, "on");
      Serial.println("MQTT publish: on");
     }
  if (relais == 0 and status_mqtt)
     {
      status_mqtt = relais;
      client.publish(mqtt_out_topic, "off");
      Serial.println("MQTT publish: off");
     }
}

#endif

void printTimeToBuffer(time_t t, char *tz)
{
  //this is needed because dayShortStr and monthShortStr obviously use the same buffer so we get
  //20:30:59 Aug 17 Aug 2018 CEST
  //instead of 
  //20:30:59 Fri 17 Aug 2018 CEST
  //if we dont do this!
  String wd(dayShortStr(weekday(t)));
  sprintf(timestrbuf,"%02d:%02d:%02d %s %02d %s %d %s",hour(t),minute(t),second(t),wd.c_str(),day(t),monthShortStr(month(t)),year(t),tz);
}
//https://www.arduinoslovakia.eu/blog/2017/7/esp8266---ntp-klient-a-letny-cas?lang=en
//Function to print time with time zone
void printTime(time_t t, char *tz, char *loc)
{
  sPrintI00(hour(t));
  sPrintDigits(minute(t));
  sPrintDigits(second(t));
  Serial.print(' ');
  Serial.print(dayShortStr(weekday(t)));
  Serial.print(' ');
  sPrintI00(day(t));
  Serial.print(' ');
  Serial.print(monthShortStr(month(t)));
  Serial.print(' ');
  Serial.print(year(t));
  Serial.print(' ');
  Serial.print(tz);
  Serial.print(' ');
  Serial.print(loc);
  Serial.println();
}
//Print an integer in "00" format (with leading zero).
//Input value assumed to be between 0 and 99.
void sPrintI00(int val)
{
  if (val < 10) Serial.print('0');
  Serial.print(val, DEC);
  return;
}

//Print an integer in ":00" format (with leading zero).
//Input value assumed to be between 0 and 99.
void sPrintDigits(int val)
{
  Serial.print(':');
  if (val < 10) Serial.print('0');
  Serial.print(val, DEC);
}
