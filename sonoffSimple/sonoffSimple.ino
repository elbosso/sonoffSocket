// Very simple example sketch for Sonoff switch as described in article "Bastelfreundlich". This sketch is just for learning purposes.
// For productive use (with MQTT and advanced features) we recommend SonoffSocket.ino in this repository.

// WPS from https://github.com/rudi48/WiFiTelnetToSerialWPS

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

//Your Wifi SSID
const char* ssid = "your_ssid";
//Your Wifi Key
const char* password = "your_key";

ESP8266WebServer server(80);

int gpio13Led = 13;
int gpio12Relay = 12;
int gpio0Switch = 0;

bool lamp = 0;
bool relais = 0;

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
    digitalWrite(gpio13Led, HIGH);
    digitalWrite(gpio12Relay, relais);
    delay(1000);
}

void doSwitchOff()
{
    relais = 0;
    digitalWrite(gpio13Led, LOW);
    digitalWrite(gpio12Relay, relais);
    delay(1000); 
}

void toggle()
{
    if(relais == 0){
      server.sendHeader("Location", String("/ein"), true);
    }else{
      server.sendHeader("Location", String("/aus"), true);
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
//  WiFi.begin("foobar",WiFi.psk().c_str()); // making sure wps happens
 
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

  
  Serial.println("Verbunden");
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.localIP());

  server.on("/", [](){
    toggle();
    server.send ( 302, "text/plain", "");  
  });
  
  server.on("/ein", [](){
    server.send(200, "text/html", "Schaltsteckdose ausschalten<p><a href=\"aus\">AUS</a></p>");
    doSwitchOn();
  });
  
  server.on("/aus", [](){
    server.send(200, "text/html", "Schaltsteckdose einschalten<p><a href=\"ein\">EIN</a></p>");
    doSwitchOff();
  });
  digitalWrite(gpio13Led, LOW);
  delay(300);
  server.begin();
  Serial.println("HTTP server started");
  
}
void loop(void){
if(digitalRead(gpio0Switch) == LOW)

  {
    if(relais == 0){
      doSwitchOn();
    }else{
      doSwitchOff();
    }

  }
  //Webserver 
  server.handleClient();
} 
