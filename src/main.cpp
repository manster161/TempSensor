#include <Arduino.h>
#include "../lib/DHT/DHT.h"
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
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

HTTPClient http;
WiFiClient client;


bool httpPost()
{

Serial.println("Connecting");
  if (client.connect(host, 80)){
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
void setup(void){
  pinMode(2, OUTPUT);
  Serial.begin(115200);

  dht.begin();


  WiFiMulti.addAP(ssid, password);
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

  //Serial.println("HTTP server started");
}




void loop(void){

  if (WiFiMulti.run() == WL_CONNECTED){
    gettemperature();
    httpPost();
    delay(20000);

  }
}
