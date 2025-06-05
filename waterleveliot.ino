#include <WiFi.h>

const char* ssid = "Hotspawt";
const char* password = "Andrewisme4";
const char* server_ip = "192.168.159.88"; // IP laptop
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
  
  // buat nyambung ke WiFi (sekaligus ngecek)
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Mencoba sambungan ke WiFi");
  }
  Serial.println("Berhasil membuat koneksi ke WiFi!");
  Serial.print("IP ESP32: ");
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
  
  // coba kirim data ke server (diattempt selama 3x percobaan)
  while(retries > 0 && !connected) {
    if (client.connect(server_ip, server_port)) {
      connected = true;
      Serial.println("Berhasil tersambung ke server");
      
      String message = "{\"rainValue\":" + String(rainValue) + "}";
      client.println(message);
      Serial.println("Data dikirim: " + message);
      
      while (client.connected() && !client.available()) {
        delay(10);
      }
      
      if (client.available()) {
        String response = client.readString();
        Serial.println("Balasan dari server: " + response);
      }
    } else {
      retries--;
      if(retries > 0) {
        Serial.println("Gagal menyambung, coba lagi...");
        delay(500);
      } else {
        Serial.println("Gagal menyambung selama 3x");
      }
    }
  }
  delay(1000); 
}
