// Very simple example sketch for Sonoff switch as described in article "Bastelfreundlich". This sketch is just for learning purposes.
// For productive use (with MQTT and advanced features) we recommend SonoffSocket.ino in this repository.

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


//Your Wifi SSID
const char* ssid = "your_ssid";
//Your Wifi Key
const char* password = "your_key";

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
NTPClient timeClient(ntpUDP);

// You can specify the time server pool and the offset, (in seconds)
// additionaly you can specify the update interval (in milliseconds).
// NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

// Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
Timezone CE(CEST, CET);

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
 
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WiFi.SSID().c_str(),WiFi.psk().c_str()); // reading data from EPROM, (last saved credentials)
//  WiFi.begin("foobar",WiFi.psk().c_str()); // making sure access point and if not configured in time (180 sec), wps happen
 
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

  server=new ESP8266WebServer(80);


  server->on("/", [](){
    toggle();
    server->send ( 302, "text/plain", "");  
  });
  
  server->on("/ein", [](){
    server->send(200, "text/html", "Schaltsteckdose ausschalten<p><a href=\"aus\">AUS</a></p>");
    doSwitchOn();
  });
  
  server->on("/aus", [](){
    server->send(200, "text/html", "Schaltsteckdose einschalten<p><a href=\"ein\">EIN</a></p>");
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
}
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
    if(relais == 0){
      doSwitchOn();
    }else{
      doSwitchOff();
    }

  }
  //Webserver 
  server->handleClient();
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
