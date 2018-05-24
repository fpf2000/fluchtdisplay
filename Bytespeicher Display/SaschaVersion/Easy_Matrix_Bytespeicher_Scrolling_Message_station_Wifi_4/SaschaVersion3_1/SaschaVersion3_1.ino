#include <WiFiServer.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiType.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiAP.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <Ticker.h>

#include <Dhcp.h>
#include <EthernetClient.h>
#include <EthernetUdp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetServer.h>

/*
Project: Wifi controlled LED matrix display
NodeMCU pins    -> EasyMatrix pins
MOSI-D7-GPIO13  -> DIN
CLK-D5-GPIO14   -> Clk
GPIO0-D3        -> LOAD
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>

// Darueber signalisieren wir, ob etwas getan werden soll.
enum Signal {
  unknown,
  DO,
  DONE
};

// Darueber signalisieren wir, ob json gepusht werden soll
volatile Signal pushSensornetDataSignal = unknown;

IPAddress sensornetHost(185, 100, 84, 206); //collector server
unsigned int sensornetPort = 9910;  //sensornet UDP Port
unsigned int localPort = 9911;     //local UDP Port
String rconUser = "tutorial";    //rcon user
String rconSecret = "peitsch";  //rcon secret
String rconCommand = "";       //rcon kommando (user:secret command)
WiFiUDP udp;
String ip("");
Ticker sensornetTicker;

#define WIFI_SSID "Freifunk Erfurt"     // insert your SSID
#define WIFI_PASS ""                    // insert your password
// ******************* String form to sent to the client-browser ************************************
String form =
  "<p>"
  "<center>"
  "<h1>Display Bytespeicher Erfurt</h1>"
  "<form action='msg'><p>Gebe hier dein Displaytext ein <input type='text' name='msg' size=100 autofocus> <input type='submit' value='Submit'></form>"
  "</center>";

ESP8266WebServer server(80);                             // HTTP server will listen at port 80
long period;
int offset=1,refresh=0;
int pinCS = 0; // Attach CS to this pin, DIN to MOSI and CLK to SCK (cf http://arduino.cc/en/Reference/SPI )
int numberOfHorizontalDisplays = 4;
int numberOfVerticalDisplays = 1;
String decodedMsg;
Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);

String tape = "Arduino";
int wait = 100; // In milliseconds

int spacer = 2;
int width = 5 + spacer; // The font width is 5 pixels

/*
  handles the messages coming from the webbrowser, restores a few special characters and
  constructs the strings that can be sent to the oled display
*/
void handle_msg() {

  matrix.fillScreen(LOW);
  server.send(200, "text/html", form);    // Send same page so they can send another msg
  refresh=1;
  // Display msg on Oled
  String msg = server.arg("msg");
  Serial.println(msg);
  decodedMsg = msg;
  // Restore special characters that are misformed to %char by the client browser
  decodedMsg.replace("+", " ");
  decodedMsg.replace("%21", "!");
  decodedMsg.replace("%22", "");
  decodedMsg.replace("%23", "#");
  decodedMsg.replace("%24", "$");
  decodedMsg.replace("%25", "%");
  decodedMsg.replace("%26", "&");
  decodedMsg.replace("%27", "'");
  decodedMsg.replace("%28", "(");
  decodedMsg.replace("%29", ")");
  decodedMsg.replace("%2A", "*");
  decodedMsg.replace("%2B", "+");
  decodedMsg.replace("%2C", ",");
  decodedMsg.replace("%2F", "/");
  decodedMsg.replace("%3A", ":");
  decodedMsg.replace("%3B", ";");
  decodedMsg.replace("%3C", "<");
  decodedMsg.replace("%3D", "=");
  decodedMsg.replace("%3E", ">");
  decodedMsg.replace("%3F", "?");
  decodedMsg.replace("%40", "@");
  //Serial.println(decodedMsg);                   // print original string to monitor
  talk2irc("#erfurt", decodedMsg);


  //Serial.println(' ');                          // new line in monitor
}


//----------------------
// setupMATRIX()
//
// Matrix initiieren ...
//----------------------
void setupMATRIX(void) {
  matrix.setIntensity(10); // Use a value between 0 and 15 for brightness

  // Adjust to your own needs
  //  matrix.setPosition(0, 1, 0); // The first display is at <0, 0>
  //  matrix.setPosition(1, 0, 0); // The second display is at <1, 0>

  // Adjust to your own needs
  matrix.setPosition(0, 0, 0); // The first display is at <0, 0>
  matrix.setPosition(1, 1, 0); // The second display is at <1, 0>
  matrix.setPosition(2, 2, 0); // The third display is at <2, 0>
  matrix.setPosition(3, 3, 0); // And the last display is at <3, 0>
  matrix.setRotation(0, 1);  // Display 1 mit 90°
  matrix.setRotation(1, 1); // Display 2 mit 90°
  matrix.setRotation(2, 1); // Display 3 mit 90°
  matrix.setRotation(3, 1); // Display 4 mit 90°
}



//-------------------
// startWIFI()
//
// Wifi starten, verbinden, ...
//-------------------
void startWIFI(void) {
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  //WiFi.mode(WIFI_STA);   ??? KvH hat das drin..
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");

  ip = ipToString(WiFi.localIP());
  Serial.println("IP address: ");
  Serial.println(ip);
}


//----------------------
// startHTTP()
//
// HTTP starten....
//----------------------
void startHTTP(void) {
    // Set up the endpoints for HTTP server,  Endpoints can be written as inline functions:
    server.on("/", []() {
      server.send(200, "text/html", form);
    });
    server.on("/msg", handle_msg);                  // And as regular external functions:
    server.begin();                                 // Start the server
    Serial.println("WebServer ready!   ");
}



//----------------------
// startUDP()
//
// UDP starten ...
//----------------------
void startUDP(void) {
    udp.begin(localPort);
    Serial.println("UDP started");
}

//-----------------
//readUDP()
//
// Nachsehen, ob wir etwas aus dem UDP Empfangspuffer lesen können
//-----------------
void readUDP() {
   char rcvbuffer[16];
  delay(2000);
  int cb = udp.parsePacket();
  if (!cb) {
    //Serial.println("no packet yet");
  }
  else {
    Serial.print("UDP packet received, length=");
    Serial.println(cb);
    udp.read(rcvbuffer, 16);
    Serial.println(rcvbuffer);
  }
}

//-----------------------
// enableSensornetPushDataSignal()
//
// Wird vom Ticker getriggert.
// Interrupt-like. So kurz wie moeglich..
//-----------------------
void enableSensornetPushDataSignal(void) {
  pushSensornetDataSignal = DO;
}

//-----------------------------
// pushSensornetData()
//
// Daten zur Sammelstelle puschen
//-----------------------------
void pushSensornetData(void) {
  rconCommand = "";
  rconCommand += rconUser;
  rconCommand += ":";
  rconCommand += rconSecret;
  rconCommand += " ";
  rconCommand += "setsensors ";

  String message = rconCommand;
  message += "{";
  message +=  "\"version\": \"0.3\",";
  message +=  "\"id\": \"f4ed3a73b46a\",";
  message +=  "\"nickname\": \"Bytespeicher_Display\",";
  message +=  "\"sensors\": ";
  message +=  "{";
  message +=   "\"display\": ";
  message +=   "[";
  message +=    "{";
  message +=     "\"name\": \"Bytespeicher Display\",";
  message +=     "\"value\": \"";
  message +=     decodedMsg;
  message +=     "\",";
  message +=     "\"unit\": \"text\"";
  message +=    "}";
  message +=   "]";
  message +=  "},";
  message +=  "\"system\": ";
  message +=  "{";
  message +=   "\"voltage\": ";
  message +=   0;
  message +=   ",";
  message +=   "\"IP\": \"";
  message +=   ipToString(WiFi.localIP());
  message +=   "\",";
  message +=   "\"timestamp\": 0,";
  message +=   "\"uptime\": 0,";
  message +=   "\"heap\": 0";
  message +=  "}";
  message += "}\n";

  int message_len = message.length() + 1;

  char msgbuffer[message_len];

  message.toCharArray(msgbuffer, message_len);

  udp.beginPacket(sensornetHost, sensornetPort);
  udp.write(msgbuffer);
  udp.endPacket();
  Serial.println(msgbuffer);

  readUDP();
}

String ipToString(IPAddress ip){
  String s="";
  for (int i=0; i<4; i++)
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  return s;
}

void talk2irc(String channel, String msg){
  rconCommand = "";
  rconCommand += rconUser;
  rconCommand += ":";
  rconCommand += rconSecret;
  rconCommand += " ";
  rconCommand += "talk";
  rconCommand += " ";

  String message = rconCommand;
  message += channel;
  message += " ";
  message += "[Bytespeicher Display] ";
  message += msg;

  int message_len = message.length() + 1;

  char msgbuffer[message_len];

  message.toCharArray(msgbuffer, message_len);

  udp.beginPacket(sensornetHost, sensornetPort);
  udp.write(msgbuffer);
  udp.endPacket();
  Serial.println(msgbuffer);

  readUDP();
}

void setup(void) {

  setupMATRIX();
  //ESP.wdtDisable();                               // used to debug, disable wachdog timer,
  Serial.begin(115200);                           // full speed to monitor
  /*
  *     Ersetzt durch Funktion
  WiFi.begin(SSID, PASS);                         // Connect to WiFi network
  while (WiFi.status() != WL_CONNECTED) {         // Wait for connection
    delay(500);
    Serial.print(".");
  }
  */
  startWIFI();
  startUDP();
  startHTTP();
  sensornetTicker.attach(60, enableSensornetPushDataSignal);
  /*
   *    Ersetzt durch Funktion??
  Serial.print("SSID : ");                        // prints SSID in monitor
  Serial.println(SSID);                           // to monitor

  char result[16];
  sprintf(result, "%3d.%3d.%1d.%3d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  Serial.println();
  Serial.println(result);
  decodedMsg = result;
  */
  decodedMsg = ip;

  /*
   *          Ersetzt druch StartWifi?
  Serial.println(WiFi.localIP());                 // Serial monitor prints localIP
  */
  Serial.print(analogRead(A0));
}


void loop(void) {

  for ( int i = 0 ; i < width * decodedMsg.length() + matrix.width() - 1 - spacer; i++ ) {
    server.handleClient();                        // checks for incoming messages
    if (refresh==1) i=0;
    refresh=0;
    matrix.fillScreen(LOW);

    int letter = i / width;
    int x = (matrix.width() - 1) - i % width;
    int y = (matrix.height() - 8) / 2; // center the text vertically

    while ( x + width - spacer >= 0 && letter >= 0 ) {
      if ( letter < decodedMsg.length() ) {
        matrix.drawChar(x, y, decodedMsg[letter], HIGH, LOW, 1);
      }

      letter--;
      x -= width;
    }

    matrix.write(); // Send bitmap to display

    delay(wait);
  }

  if (pushSensornetDataSignal == DO)
  {
    pushSensornetData();
    pushSensornetDataSignal = DONE;
  }
}
void setup() {
  pinMode(ESP8266_LED, OUTPUT);
  pinMode(D1, INPUT_PULLUP);
  pinMode(D2, INPUT_PULLUP);
}

// the loop function runs over and over again forever
void loop() {
  if(digitalRead(D1)){
    digitalWrite(ESP8266_LED, HIGH);
  }else{
    digitalWrite(ESP8266_LED, LOW);
  }

  if(digitalRead(D2)){
    digitalWrite(ESP8266_LED, HIGH);
  }else{
    digitalWrite(ESP8266_LED, LOW);
  }

}
