/*
   Arduino IDE Settings (Arduino AVR Boards 1.6.23):
     Board: Generic ESP8266 Module (ESP-01)
     Builtin LED: 2
     Upload Speed: 115200
     CPU Frequency: 80MHz
     Crystal Frequency: 26MHz
     Flash Size: 1024K (256K SPIFFS - User selected size of flash FS)
     Flash Mode: DIO
     Flash Frequency: 40MHz
     Reset Method: no dtr (aka ck)
     Debug Port: Disabled
     Debug Level: None
     lwIP Variant: v2 Lower Memory
     vTables: Flash
     Erase Flash: Only Sketch
     Port: /dev/ttyUSB0
     Programmer: AVRISP mkII

   Programming:
     1. CTRL-R to verify/compile code
     2. Remove USB programmer from computer
     3. Switch USB programmer to "PROG"
     4. Insert ESP-01 module into USB programmer
     5. Insert USB programmer into computer
     6. CTRL-U to upload code

   Description:
     Based on Arduino web server example

     http://arduino.stackexchange.com/questions/13388/arduino-webserver-faster-alternative-to-indexof-for-parsing-get-requests

   Notes:
     1. Close Serial Monitor window before programming.
     2. For ESP-01S v1.0 relay board you may need to add jump wire between Vcc (pin 8) and CH_EN (pin 4).
     3. DO NOT REMOVE R2 from the relay board, you'll need it for the GPIO3 mod. 

   Use Arduino 2.4.0-rc2 or higher to build (Tools->Board->Boards Manager)

     https://github.com/esp8266/Arduino/releases

   History:
     20221016 - Initial release with GPIO3 hardware hack instead of GPIO0 for relay.

*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// GPIO Assignments
#define NA 0          // No longer used (https://github.com/IOT-MCU/ESP-01S-Relay-v4.0/issues/1)
#define LED 1         // Blue LED connected to GPI01
#define RELAY 3       // Relay connected to GPIO3

const char* ssid = "WIFISSID";
const char* password = "WIFIPASSWORD";

char HostName[32];
const char compile_date[] = __DATE__ " " __TIME__;
unsigned long endTime = 0;
unsigned long runTime = 0;
unsigned long strTime = 0;
unsigned long tmpTime = 0;
unsigned long rstTime = (5 * 60 * 1000);  // Reset timeout in millis if WiFi connection fails (5 mins)
String s, w;

ADC_MODE(ADC_VCC);          // Required for ESP.getVcc() to work
ESP8266WebServer server(80);

void actRelay(String v) {
  s = "<kbd>RELAY is ";
  if (v.startsWith("on") || v == "1") {
    if (!(digitalRead(RELAY))) {
      for (int i = 0; i < server.args(); i++) {
        if (server.argName(i) == "t") { runTime = server.arg(i).toInt() * 1000; }
      }
      if (runTime == 0) { runTime = 1800000; }  // Default to 30 minutes (1800 seconds) if no time is provided
      strTime = millis();
      endTime = strTime + runTime;
      digitalWrite(RELAY, HIGH);
    }
    s += "on with " + String((endTime - millis()) / 1000) + " seconds remaining";
  } else if (v.startsWith("of") || v == "0") {
    digitalWrite(RELAY, LOW);
    s += "off";
    runTime = 0;
  } else {
    (digitalRead(RELAY)) ? s += "on with " + String((endTime - millis()) / 1000) + " seconds remaining" : s += "off";
  }

  if (v.startsWith("st") || v == "2") {    // For quick query to prevent parsing data.
    server.send(200, "text/plain", (runTime == 0) ? "0" : String((endTime - millis()) / 1000));
    return;
  }
  
  server.send(200, "text/html", s + String(". Click <a href='/a?rel=") + ((digitalRead(RELAY)) ? "off" : "on&t=1800") + String("'>here</a> to turn ") + ((digitalRead(RELAY)) ? "off" : "on") + String(".</kbd>"));
}

void handle_Action() {
  digitalWrite(LED, LOW);   // Turn LED on to indicate action being requested
  for (int i = 0; i < server.args(); i++) {
    if (server.argName(i).startsWith("rel")) { actRelay(server.arg(i)); }
  }
  digitalWrite(LED, HIGH);  // Turn LED off when done processing actions
}

void handle_Info() {
  s = "ESP8266\n\nCPU Info/Stats";
  s += "\n  Chip ID: " + String(ESP.getChipId());
  s += "\n  Free heap size: " + String(ESP.getFreeHeap()) + " Bytes";
  s += "\n  Instruction Cycle Count: " + String(ESP.getCycleCount());
  s += "\n  Supply Voltage: " + String(ESP.getVcc()) + " mV";
  s += "\n  Flash Chip ID: " + String(ESP.getFlashChipId());
  s += "\n  Flash Chip Size: " + String(ESP.getFlashChipSize()) + " Bytes";
  s += "\n  Real Chip Size: " + String(ESP.getFlashChipRealSize()) + " Bytes";
  s += "\n  Flash Chip Speed: " + String(ESP.getFlashChipSpeed()) + " Hz";
  s += "\n  Uptime: " + String(millis()) + " ms";
  s += "\n\nNetwork Info";
  s += "\n  WiFi Network: " + String(ssid);
  s += "\n  Hostname: " + String(HostName);
  s += "\n  MAC Address: " + WiFi.macAddress();
  s += "\n  IP address: " + WiFi.localIP().toString();
  s += "\n\nReason for last reset? " + String(ESP.getResetReason());
  s += "\n\nCode Tracing";
  s += "\n  Build version: " + String(compile_date);
  s += "\n  strTime: " + String(strTime);
  s += "\n  runTime: " + String(runTime);
  s += "\n  endTime: " + String(endTime);
  s += "\n  rstTime: " + String(rstTime);
  s += "\n  tmpTime : " + String(tmpTime);
  s += "\n  Relay: " + String(((digitalRead(RELAY)) ? "on" : "off"));

  server.send(200, "text/plain", s);
}

void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}

void handle_Restart() {
  server.sendHeader("Location", "/");
  server.sendHeader("Cache-Control", "no-cache");
  server.sendHeader("Set-Cookie", "ESPSESSIONID=0");
  server.send(301);
  ESP.restart();
}

void handle_Scan() {
  server.send(200, "text/html", String("<kbd>") + ((WiFi.scanComplete() < 0) ? "Scan has not yet been done." : w) + String("\n<br>\n<br>Click <a href='/wifiscan'>here</a> to scan for WiFi networks."));
}

void handle_WiFiScan() {
  server.sendHeader("Location", "/scan");
  server.sendHeader("Cache-Control", "no-cache");
  server.sendHeader("Set-Cookie", "ESPSESSIONID=0");
  server.send(301);

  WiFi.disconnect();
  delay(100);

  int n = WiFi.scanNetworks(false, true);
  if (n == 0) {
    w = "No networks found";
  } else {
    w = String(n) + " network(s) found\n";
    for (int i = 0; i < n; ++i) {
      w += "\n" + String(i + 1) + ": ";
      w += String(WiFi.SSID(i));
      w += " [" + String(WiFi.BSSIDstr(i)) + "] ";
      w += "Ch: " + String(WiFi.channel(i));
      w += " (" + String(WiFi.RSSI(i)) + " dBm)";
      w += " Enc: ";

      switch (WiFi.encryptionType(i)) {
        case 2:
          w += "WPA/PSK";
          break;
        case 4:
          w += "WPA2/PSK";
          break;
        case 5:
          w += "WEP";
          break;
        case 7:
          w += "NONE";
          break;
        case 8:
          w += "WPA/WPA2/PSK";
          break;
        default:
          w += "Unknown";
          break;
      }
      delay(10);
    }
  }
  w.replace("\n", "\n<br>");
  fnWiFiConnect(100);
}

void fnSetMAC() {
  // Custom MAC address (first octet odd = unicast, even = multicast which is NOT supported by the ESP8266)
  // Set MAC before WiFi.begin()

  uint8_t newMAC[] {0x00, 0x24, 0x68, 0x01, 0x02, 0x03};   // Ag well prod controller MAC

  wifi_set_macaddr(STATION_IF, &newMAC[0]);
  sprintf(HostName, "ESP-%02X%02X%02X", newMAC[3], newMAC[4], newMAC[5]);
}

void fnWebServerStart() {
  server.on("/", []() { server.send(200, "text/plain", HostName); });
  server.on("/a", handle_Action);
  server.on("/info", handle_Info);
  server.on("/restart", handle_Restart);
  server.on("/scan", handle_Scan);
  server.on("/wifiscan", handle_WiFiScan);
  server.onNotFound(handle_NotFound);
  server.begin();
}

void fnWiFiConnect(int i) {
  WiFi.begin(ssid, password);
  digitalWrite(LED, LOW);
  tmpTime = millis() + rstTime;   // minutes * seconds * milliseconds
  while (WiFi.status() != WL_CONNECTED) {
    if (tmpTime < millis()) { ESP.restart(); }    // restart if unable to connect
    delay(i);
    ((digitalRead(LED)) == LOW) ? digitalWrite(LED, HIGH) : digitalWrite(LED, LOW);
  }
  digitalWrite(LED, HIGH);    // Turn off LED when WiFi joined
}

void setup(void) {
  // Initialize GPIO pins
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);   // LOW = open circuit at relay
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);     // Turn on LED on power up

  WiFi.mode(WIFI_STA);        // Must be called before MAC address is changed!
  fnSetMAC();
  WiFi.hostname(HostName);

  fnWiFiConnect(100);
  fnWebServerStart();
}

void loop(void) {
  server.handleClient();
  if (strTime > millis()) { endTime = 4294967295 - strTime + runTime; } // Deal with millis() rollover (unlikely to occur)
  if (endTime <= millis()) { actRelay("off"); }
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    fnWiFiConnect(500);
  } else {
    digitalWrite(LED, HIGH);
  }
}
