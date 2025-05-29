#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "ThingSpeak.h"

#define RAIN_SENSOR_PIN D5

const char* ssid = "NAMA_WIFI_KAMU";
const char* password = "PASSWORD_WIFI_KAMU";
const char* thingSpeakApiKey = "API_KEY_THINGSPEAK_KAMU";
const unsigned long channelID = 123456;

WiFiClient client;

void setup() {
  Serial.begin(9600);
  pinMode(RAIN_SENSOR_PIN, INPUT);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  ThingSpeak.begin(client);
}

void loop() {
  int rainDetected = digitalRead(RAIN_SENSOR_PIN);
  if (rainDetected == LOW) {
    Serial.println("Hujan terdeteksi");
    ThingSpeak.writeField(channelID, 1, 1, thingSpeakApiKey);
  } else {
    Serial.println("Tidak ada hujan");
    ThingSpeak.writeField(channelID, 1, 0, thingSpeakApiKey);
  }
  delay(15000);
}
