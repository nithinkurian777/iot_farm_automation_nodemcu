#include "DHT.h"
#define DHTPIN 2
#define MOISTUREPIN 3
#define DHTTYPE DHT11 
DHT dht(DHTPIN, DHTTYPE);
void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(MOISTUREPIN,INPUT);
}

void loop() {
  int moistureValue = digitalRead(MOISTUREPIN);
  Serial.println("Moisture Level: ");
  Serial.print(moistureValue);
  Serial.println();
 float h = dht.readHumidity(); 
 float t = dht.readTemperature(); 
 if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
}

}
