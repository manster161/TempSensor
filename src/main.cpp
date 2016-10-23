#include <Arduino.h>
#include "../lib/DHT/DHT.h"
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include "../lib/TickerScheduler/TickerScheduler.h"
#include "config.h"

#define DHTTYPE DHT11
#define DHTPIN  2

DHT dht(DHTPIN, DHTTYPE, 11);

unsigned long previousMillis = 0;        // will store last temp was read
const long interval = 2000;
char buffer[1000];
float humidity, temp;

String webString="";
ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);
HTTPClient http;
WiFiClient client;

TickerScheduler scheduler(1);

int updateTime;

void scanNetworks(){

  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("scan start");

 // WiFi.scanNetworks will return the number of networks found
 int n = WiFi.scanNetworks();
 Serial.println("scan done");
 if (n == 0)
   Serial.println("no networks found");
 else
 {
   Serial.print(n);
   Serial.println(" networks found");
   for (int i = 0; i < n; ++i)
   {
     // Print SSID and RSSI for each network found
     Serial.print(i + 1);
     Serial.print(": ");
     Serial.print(WiFi.SSID(i));
     Serial.print(" (");
     Serial.print(WiFi.RSSI(i));
     Serial.print(")");
     Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
     delay(10);
   }
}
}

bool IsNumeric(String s){
  for (int i = 0; i < s.length(); i++){
    if (!isDigit(s[0]))
    return false;
  }
  return true;
}

void handle_root() {
  server.send(200, "text/plain", "Hello from the weather esp8266, read from /temp or /humidity");
  delay(100);
}


bool httpPost()
{

Serial.println("Connecting");
  if (client.connect(host.c_str(), 80)){
    String postStr = apiKey;
    postStr +="&field1=";
    postStr += String(temp);
    postStr +="&field2=";
    postStr += String(humidity);
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: "+ String(apiKey) + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
    client.stop();
    Serial.println("Posted:" + postStr);
  }
  else
  {
    Serial.println("Connect fail");
  }
}




void gettemperature() {
  // Wait at least 2 seconds seconds between measurements.
  // if the difference between the current time and last time you read
  // the sensor is bigger than the interval you set, read the sensor
  // Works better than delay for things happening elsewhere also
  unsigned long currentMillis = millis();

  if(currentMillis - previousMillis >= interval) {
    // save the last time you read the sensor
    previousMillis = currentMillis;

    // Reading temperature for humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
    humidity = dht.readHumidity();          // Read humidity (percent)
    temp = dht.readTemperature(false);
    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temp)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
  }
}

void sendData(){
  gettemperature();
  httpPost();

}

void setupNetwork(){

  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  delay(100);

  WiFiMulti.addAP(ssid.c_str(), password.c_str());
  http.setReuse(true);

  Serial.print("\n\r \n\rWorking to connect");

  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("DHT Weather Reading Server");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setupServer(){
  server.on("/", handle_root);

  server.on("/temp", [](){  // if you add this subdirectory to your webserver call, you get text below :)
          // read sensor
    webString="Temperature: "+String((int)temp)+"c";   // Arduino has a hard time with float to string
    server.send(200, "text/plain", webString);            // send to someones browser when asked
  });

  server.on("/humidity", [](){  // if you add this subdirectory to your webserver call, you get text below :)
             // read sensor
    webString="Humidity: "+String((int)humidity)+"%";
    server.send(200, "text/plain", webString);               // send to someones browser when asked
  });


  server.on("/settings", [](){  // if you add this subdirectory to your webserver call, you get text below :)

    String state = server.arg("updatetime");
    if (IsNumeric(state)){
       updateTime = state.toInt();
       bool removeSuccess =  scheduler.remove(0);

       if (removeSuccess)
        Serial.println("Successfully removed scheduler");

        if (scheduler.add(0, updateTime, sendData))
          Serial.println("Successfully added scheduler");
    }
    webString="Update time: "+String((int)updateTime/1000)+"s";
    server.send(200, "text/plain", webString);

  });


  server.on("/network", [](){

    String s = server.arg("ssid");
    String p = server.arg("password");

    if (s.length() > 0 && p.length() > 0){
      Serial.println("Changing network to SSID=" + s + " Password=" + p);
      ssid = s;
      password = p;
      setupNetwork();
    }

  });


  server.begin();
  Serial.println("HTTP server started");

}



void setup(void){
  pinMode(2, OUTPUT);
  Serial.begin(115200);

  dht.begin();

  setupNetwork();
  setupServer();

  updateTime = 60000;

  scheduler.add(0, updateTime, sendData);

}




void loop(void){

  if (WiFiMulti.run() == WL_CONNECTED){
    scheduler.update();
    server.handleClient();
  }
}
