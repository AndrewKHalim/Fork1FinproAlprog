#define RAIN_SENSOR_PIN 15  // use the GPIO number

void setup() {
  Serial.begin(9600);
  pinMode(RAIN_SENSOR_PIN, INPUT);
}

void loop() {
  int rainValue = digitalRead(RAIN_SENSOR_PIN);
  Serial.println(rainValue);
  delay(100);
}
