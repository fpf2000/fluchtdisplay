
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <FS.h>

#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h> 
#include <stdio.h>
#include "Streaming.h"
#include <String.h>

#define SLEEP_DELAY_IN_SECONDS  30
#define NEOPIN           D3
#define NUMPIXELS      4
#define Button1 D5
#define Button2 D6

const char* clientName;


char ssid[40] = "Freifunk Erfurt";
char password[40] = "";
char devicename[40] ="";

char mqtt_server[40] = "192.168.1.110";
char mqtt_username[40] = "test";
char mqtt_password[40] = "testpwd";

char mqtt_topictemp[40] = "sensors/NAME/temperature";
char mqtt_topicled[40] = "sensors/NAME/led";
char mqtt_topicstate[40] = "sensors/NAME/state";

char mqtt_subtopicled[40] = "action/NAME/led";
char mqtt_subtopicstate[40] = "action/NAME/state";

int btnVal1 = 0;
int btnVal2 = 0;
char newstate[10] = "00";
char oldstate[10] = "00";

int redVal=128;
int greVal=128;
int bluVal= 128;

int midpix=0;
int mixpixp=0;
int mixpixn=0;
int realpix=0;
char rgbVal[16] ="";
unsigned long mytime;

WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, NEOPIN, NEO_GRB + NEO_KHZ800);

char ledString[16];
char stateString[6];
String sLedString;
String sStateString;


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Message arrived [" + String(topic)+"] ");
  Serial.println("Length: " + String(length,DEC));
  char message_buff[100];
  int i;
    for (i = 0; i < length; i++){
      message_buff[i] = payload[i];    
  }
  message_buff[i] = '\0';
  Serial.println();
 String msgString = String(message_buff);
 Serial.println("Payload: " + msgString);
  
     if(String(topic)==String(mqtt_subtopicstate))
     {
        //int state = String(buffer).toInt();
      }

     if(String(topic)==String(mqtt_subtopicled))
     {
      Serial.println("led:" +msgString);
      msgString.toCharArray(ledString,16);
      int number = (int) strtol( ledString, NULL, 16);
    // Split them up into r, g, b values
      redVal  = number >> 16;
      greVal = number >> 8 & 0xFF;
      bluVal = number & 0xFF;
      
     }
     
  
   
   
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientName, mqtt_username, mqtt_password)) 
    {
      Serial.println("connected");
    }
    else
    {
      
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      Serial.println(clientName);
      Serial.println(mqtt_username);
      Serial.println(mqtt_password);
   
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}


void initled()
{
  for(int i=0;i<NUMPIXELS;i++){
    if(i%2)
    {
      pixels.setPixelColor(i, pixels.Color(0,0,111)); // Moderately bright green color.
    }
    else if(i%3)
    {
      pixels.setPixelColor(i, pixels.Color(0,111,0)); // Moderately bright green color.

    }
    else if(i%5)
    {
      pixels.setPixelColor(i, pixels.Color(111,0,0)); // Moderately bright green color.
    }
    else
    {
      pixels.setPixelColor(i, pixels.Color(111,111,111)); // Moderately bright green color.
    }
  }
  pixels.show();

}


void StripColor(int r,int g,int b)
{
        for(int i=0;i<NUMPIXELS;i++)
        {
            pixels.setPixelColor(i, pixels.Color(r,g,b)); // Moderately bright green color.
        }
      pixels.show();
}



void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<pixels.numPixels(); i++) {
      pixels.setPixelColor(i, Wheel((i+j) & 255));
    }
    pixels.show();
    delay(wait);
  }
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void kitt()
{
  pixels.clear();
  midpix=NUMPIXELS/2;

  mixpixp=midpix;
  mixpixn=midpix-1;

    for(int i=0;i<=midpix;i++)
    {

          pixels.setPixelColor(mixpixp, pixels.Color(177,0,10)); // Moderately bright green color.
          pixels.setPixelColor(mixpixn, pixels.Color(177,0,10)); // Moderately bright green color.
          mixpixn-=1;
          mixpixp+=1;

          pixels.show();
          delay(mixpixn+1);
    }
    mixpixn=0;
    mixpixp=NUMPIXELS;
    for(int i=0;i<=midpix;i++)
    {
          pixels.setPixelColor(mixpixp, pixels.Color(redVal,greVal,bluVal)); // Moderately bright green color.
          pixels.setPixelColor(mixpixn, pixels.Color(redVal,greVal,bluVal)); // Moderately bright green color.
          mixpixn+=1;
          mixpixp-=1;
          pixels.show();
          delay(mixpixn+1);
    }

}

void readfile(String file)
{
    if (SPIFFS.exists(file))
    {
      //file exists, reading and loading
      Serial.println("Reading config file");
      
      Serial.println(file);
      File configFile = SPIFFS.open(file, "r");
      if (configFile)
      {
        Serial.println("Opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success())
        {
          Serial.println("parsed json");
          
          strcpy(ssid, json["ssid"]);
          strcpy(password, json["password"]);
          
          strcpy(devicename, json["devicename"]);
          
          Serial.println(ssid);
          Serial.println(password);
          Serial.println(devicename);
          
        String sName;

        sName=mqtt_topictemp;
        sName.replace("NAME",devicename);
        strcpy(mqtt_topictemp ,sName.c_str());
        
        sName=mqtt_topicled;
        sName.replace("NAME",devicename);
        strcpy(mqtt_topicled ,sName.c_str());

        sName=mqtt_topicstate;
        sName.replace("NAME",devicename);
        strcpy(mqtt_topicstate ,sName.c_str());
        
        sName=mqtt_subtopicled;
        sName.replace("NAME",devicename);
        strcpy(mqtt_subtopicled ,sName.c_str());

        sName=mqtt_subtopicstate;
        sName.replace("NAME",devicename);
        strcpy(mqtt_subtopicstate ,sName.c_str());
              
        } 
      }
    }
    else
    {
       
    }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(D5,INPUT_PULLUP);
  pinMode(D6,INPUT_PULLUP);
  
  
  SPIFFS.begin();

  Serial.println("Booting");

 if (SPIFFS.exists("/config.json"))
 {
   readfile("/config.json");
 }
 else
 {

  if(SPIFFS.format())
  {
      DynamicJsonBuffer jsonBuffer;
      Serial.println("Format successful"); 
      JsonObject& json = jsonBuffer.createObject();
      json["devicename"] = "MSStatus";
      
      json["ssid"] = "Makerspace";
      json["password"] = "correcthorsebatterystaple";

       File configFile = SPIFFS.open("/config.json", "w");
       if (!configFile)
       {
        Serial.println("Failed to open config file for writing");
       
       }
       else
       {
        json.printTo(Serial);
        json.printTo(configFile);
        configFile.close();
       }
  }
  else
  {
      Serial.println("Format failed"); 
  }   
 }
 
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  
  
  uint8_t mac[6];
  WiFi.macAddress(mac);
 clientName=strdup(devicename);
 ArduinoOTA.setHostname(clientName);

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  pixels.begin();
  pixels.clear();
  initled(); 
  rainbow(10);
 
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  ArduinoOTA.handle();
  
  if (!client.connected())
  {
    delay(1000);
    reconnect();
    if (client.subscribe(mqtt_subtopicled))
    {
      Serial << "Successfully subscribed: " << mqtt_subtopicled << endl;    
    }
    
  }
  client.loop();

// if (millis() > (mytime + 30000)) {
  if (millis() > (mytime + 500)) {
        mytime = millis();
    
 /*
  Serial << "Sending temperature: " << temperatureString << endl;
  client.publish(mqtt_topictemp, temperatureString);
  */
  if(digitalRead(Button1))
  {
    btnVal1=1;
  }
  else
  {
    btnVal1=0;
  }
  
  if(digitalRead(Button2))
  {
    btnVal2=1;
  }
  else
  {
    btnVal2=0;
  }
 
  sprintf(newstate, "%d%d", btnVal1,btnVal2);

  if(strcmp(newstate,"11") == 0 )
  {
    redVal=0;
    greVal=255;
    bluVal=0;
  }
  else if(strcmp(newstate,"01")  == 0)
  {
    redVal=0;
    greVal=255;
    bluVal=255;
  
  }
  else if(strcmp(newstate,"10") == 0)
  {
     redVal=0;
    greVal=0;
    bluVal=255;
  }
  else
  {
     redVal=255;
    greVal=0;
    bluVal=0;
   

  }
  StripColor(redVal,greVal,bluVal);
  
  
    if(strcmp(oldstate,newstate) == 0)
    {
      
    }
    else
    {
      Serial.print( "Sending State: ");
      Serial.println(newstate);
      Serial.print( "Old State: ");
      Serial.println(oldstate);
      strcpy(oldstate,newstate);
      client.publish(mqtt_topicstate ,newstate,true); 
     }
  }
  delay(500);
}
