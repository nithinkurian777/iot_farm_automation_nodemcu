#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "secrets.h"
#include "DHT.h"
 
#define DHTPIN D4        // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11
#define MOISTURE_PIN D5
#define MOT1 D6
#define MOT2 D7
DHT dht(DHTPIN, DHTTYPE);
 
float h ;
float t;
int m;
unsigned long lastMillis = 0;
unsigned long previousMillis = 0;
const long interval = 5000;
const char* serverName = "http://node-express-env.eba-qspymxrp.ap-south-1.elasticbeanstalk.com/api/insertData";
 
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"
 
WiFiClientSecure net;
 
BearSSL::X509List cert(cacert);
BearSSL::X509List client_crt(client_cert);
BearSSL::PrivateKey key(privkey);
 
PubSubClient client(net);
 
time_t now;
time_t nowish = 1510592825;
 
 
void NTPConnect(void)
{
  Serial.print("Setting time using SNTP");
  configTime(TIME_ZONE * 3600, 0 * 3600, "pool.ntp.org", "time.nist.gov");
  now = time(nullptr);
  while (now < nowish)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("done!");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}
 
 
void messageReceived(char *topic, byte *payload, unsigned int length)
{
  String msg = "";
  Serial.print("Received [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    msg+=(char)payload[i];
  }
  if(msg == "m1-1"){
    digitalWrite(MOT1,LOW); 
    Serial.println("Motor 1 ON");   
  }else if(msg == "m1-0"){
    digitalWrite(MOT1,HIGH);
    Serial.println("Motor 1 OFF");  
  }
  else if(msg == "m2-1"){
    digitalWrite(MOT2,LOW); 
    Serial.println("Motor 2 ON");     
  }else if(msg == "m2-0"){
    digitalWrite(MOT2,HIGH);
    Serial.println("Motor 2 OFF");  
  }
  Serial.println();
}
 
 
void connectAWS()
{
  delay(3000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  Serial.println(String("Attempting to connect to SSID: ") + String(WIFI_SSID));
 
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }
 
  NTPConnect();
 
  net.setTrustAnchors(&cert);
  net.setClientRSACert(&client_crt, &key);
 
  client.setServer(MQTT_HOST, 8883);
  client.setCallback(messageReceived);
 
 
  Serial.println("Connecting to AWS IOT");
 
  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(1000);
  }
 
  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }
  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
 
  Serial.println("AWS IoT Connected!");
}
 
 
void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["time"] = millis();
  doc["humidity"] = h;
  doc["temperature"] = t;
  doc["moisture"] = m;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
 
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}
 void publishHTTP(){
  //HTTP Part
  WiFiClient client2;
  HTTPClient http;

      http.begin(client2, serverName);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      char temp[10];
      char humidity[10];
      char moisture[5];
      sprintf(temp,"%f",t);
      sprintf(humidity,"%f",h);
      sprintf(moisture,"%d",m);

      String httpRequestData = "temperature=" + String(temp) + "&humidity=" + String(humidity)+"&moisture=" + String(moisture);           
      int httpResponseCode = http.POST(httpRequestData);
           
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      http.end();

//End HTTP Part
 
  }
 
void setup()
{
  Serial.begin(115200);
  connectAWS();
  dht.begin();
  pinMode(MOISTURE_PIN,INPUT);
  pinMode(MOT1,OUTPUT);
  pinMode(MOT2,OUTPUT);
  digitalWrite(MOT1,HIGH);
  digitalWrite(MOT2,HIGH);
}
 
 
void loop()
{
  h = dht.readHumidity();
  t = dht.readTemperature();
  m = digitalRead(D5);
 
  if (isnan(h) || isnan(t) )  // Check if any reads failed and exit early (to try again).
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
 
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.println(F("°C "));
  delay(2000);


  now = time(nullptr);
 
  if (!client.connected())
  {
    connectAWS();
  }
  else
  {
    client.loop();
    if (millis() - lastMillis > 5000)
    {
      lastMillis = millis();
      publishMessage();
      publishHTTP();
    }
  }
}
