#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const char* mqtt_server = "test.mosquitto.org";
const int Push_button_pin = 13;    
const int led =  14;
int appuyer = 0;
int allumer = 0;
int allumerPrecedent = 1;
float tempPrecedent = 0;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

#define ONE_WIRE_BUS 12 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void temperature() {
  Serial.println("Dallas Temperature IC Control Library Demo");
  sensors.begin();
}

void setup_wifi() {
  Serial.begin(115200);
  WiFiManager wifiManager;
  wifiManager.autoConnect("ESP8266AP"); 
  Serial.println("Connected to WiFi");
}

void callback(char* topic, byte* payload, unsigned int length) {
  char retour[100]; 
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    retour[i] = (char)payload[i];
  }
  if (strcmp(retour , "off") == 0) {
    Serial.println("OFF");
    allumer = 0; 
  } else if (strcmp(retour , "on") == 0) {
    Serial.println("ON");
    allumer = 1; 
  }
}

void message_on() {
  client.publish("outTopicV2.0", "on");
}

void message_off(){
  client.publish("outTopicV2.0", "off");
}

void get_temp(){
  sensors.requestTemperatures();
  float number = sensors.getTempCByIndex(0);  
  char temp[20];
  if (number != tempPrecedent){
    int len = snprintf(temp, sizeof(temp), "%.2f", number);
    client.publish("outTopicV2.0.t",temp);
    tempPrecedent = number;
  } 
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.publish("outTopicV2.0", "connect");
      client.subscribe("inTopicV2.0");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  temperature();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  pinMode(Push_button_pin, INPUT);
  pinMode(led, OUTPUT);
  allumer = 0;
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  get_temp();

  appuyer = digitalRead(Push_button_pin);
  if (appuyer == 1) {
    if (allumer != allumerPrecedent){
      if (allumer == 1) {
        allumer = 0 ;
        message_off();
      } else {
        allumer = 1;
        message_on();
      }
    }
  } else {
   if (allumer == 1){
     digitalWrite(led, HIGH);
     allumerPrecedent = 0;
  } else{
    digitalWrite(led, LOW);
    allumerPrecedent = 1;
  }
  }
  if (allumer == 1){
    digitalWrite(led, HIGH);
    message_on();
     
  } else{
    digitalWrite(led, LOW);
    message_off();
  }
}