#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
 
Adafruit_ADS1115 ads(0x48);
WiFiServer httpServer(80);
String header;
unsigned long currentTime = millis();
unsigned long previousTime = 0; 
const long timeoutTime = 2000;
const float offset = 2432;
const float multiplier = 0.0001875F;

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setup(void)
{
  Serial.begin(9600);
  
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setHostname("PRESSURE-SENSOR");

  if(!wifiManager.autoConnect("PRESSURE-AP")) {
    Serial.println("Failed to connect and hit timeout");
    ESP.reset();
    delay(1000);
  } 
  Serial.println("Connected to WiFi!");
  httpServer.begin();
  ads.begin();
}
 
void loop(void)
{
  WiFiClient client = httpServer.available();
  WiFi.softAPdisconnect(true);
  WiFi.enableAP(false);

  if (client) {
    Serial.println("New Client."); 
    String currentLine = "";
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();         
      if (client.available()) {             
        char c = client.read();             
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}</style></head>");
            client.println("<body><h1>Pressure Sensor Web Server</h1>");
            client.println("<p><b>Pressure:</b> " + String(getPressure()) + " psi</p>");
            client.println("</body></html>");
            client.println();
            break;
          } else { 
            currentLine = "";
          }
        } else if (c != '\r') {  
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

float getPressure(){
  int16_t adc0;

  // GET AVG
  adc0 = 0;
  Serial.println("Getting sample of analog readings...");
  for (float x=0; x < 60; x++) adc0 += ads.readADC_SingleEnded(0);
  Serial.println("Got readings!");
   
  // CONVERT TO VOLTS 
  float voltage = ((adc0 - offset) * multiplier) / 1000;
  Serial.print("Volts: ");
  Serial.println(voltage,3);

  float psi = mapFloat(voltage, 0.0, 4.5, 0.0, 232.06);
  Serial.print("PSI: ");
  Serial.println(psi,3);

  return psi;
}

float mapFloat(float value, float fromLow, float fromHigh, float toLow, float toHigh)
{
  return (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
}
