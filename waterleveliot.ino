#include <WiFi.h>

const char* ssid = "Hotspawt";
const char* password = "Andrewisme4";
const char* server_ip = "192.168.159.88"; // Your laptop's IP address
const int server_port = 8080;

#define RainSensorPin 33 
#define RedLEDPin 13
#define GreenLEDPin 26
#define BlueLEDPin 14

WiFiClient client;

void setup() {
  Serial.begin(9600);
  pinMode(RainSensorPin, INPUT);
  pinMode(GreenLEDPin, OUTPUT);
  pinMode(BlueLEDPin, OUTPUT);
  pinMode(RedLEDPin, OUTPUT);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  int rainValue = analogRead(RainSensorPin);
  if(rainValue >= 3000){
    Serial.println("0");
    digitalWrite(RedLEDPin, HIGH);
    digitalWrite(GreenLEDPin, LOW);
    digitalWrite(BlueLEDPin, LOW);
  }
  else if(rainValue < 3000 && rainValue > 1100){
    Serial.println("1");
    digitalWrite(RedLEDPin, LOW);
    digitalWrite(GreenLEDPin, HIGH);
    digitalWrite(BlueLEDPin, LOW);
  }
  else if(rainValue <= 1100){
    Serial.println("2");
    digitalWrite(RedLEDPin, HIGH);
    digitalWrite(GreenLEDPin, LOW);
    digitalWrite(BlueLEDPin, HIGH);
  }
  

  int retries = 3;
  bool connected = false;
  
  // Connect to server and send data (with retry)
  while(retries > 0 && !connected) {
      if (client.connect(server_ip, server_port)) {
        connected = true;
      Serial.println("Connected to server");
      
      String message = "{\"rainValue\":" + String(rainValue) + "}";
      
      // Send data
      client.println(message);
      Serial.println("Data sent: " + message);
      
      // Wait for response
      while (client.connected() && !client.available()) {
        delay(10);
      }
      
      if (client.available()) {
        String response = client.readString();
        Serial.println("Server response: " + response);
      }
    } else {
      retries--;
      if(retries > 0) {
        Serial.println("Connection failed, retrying...");
        delay(500);
      } else {
        Serial.println("Connection failed after retries");
      }
    }
  }
  delay(1000); 
}
