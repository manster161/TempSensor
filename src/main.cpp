#include <Arduino.h>
#include "../lib/DHT/DHT.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>


#define DHTTYPE DHT11
#define DHTPIN  2


DHT dht(DHTPIN, DHTTYPE, 11);

unsigned long previousMillis = 0;        // will store last temp was read
const long interval = 2000;
char buffer[1000];
float humidity, temp;
char thingSpeakAddress[] = "api.thingspeak.com";
char writeAPIKey[] = "EDBWQ9ANCKM0E184";
ESP8266WebServer server(80);

String webString="";
char ssid[] = "Mansternet";
char password[] = "snoppsnopp666";


void handle_root() {
  server.send(200, "text/plain", "Hello from the weather esp8266, read from /temp or /humidity");
  delay(100);
}

void postdata(){

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


  WiFi.begin(ssid, password);
  Serial.print("\n\r \n\rWorking to connect");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("DHT Weather Reading Server");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handle_root);

  server.on("/temp", [](){  // if you add this subdirectory to your webserver call, you get text below :)
    gettemperature();       // read sensor
    webString="Temperature: "+String((int)temp)+"c";   // Arduino has a hard time with float to string
    server.send(200, "text/plain", webString);            // send to someones browser when asked
  });

  server.on("/humidity", [](){  // if you add this subdirectory to your webserver call, you get text below :)
    gettemperature();           // read sensor
    webString="Humidity: "+String((int)humidity)+"%";
    server.send(200, "text/plain", webString);               // send to someones browser when asked
  });

  server.begin();
  Serial.println("HTTP server started");
}




void loop(void){

    server.handleClient();
}
