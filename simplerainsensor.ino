#define RainSensorPin 15  
#define RedLEDPin 4
#define GreenLEDPin 17
#define BlueLEDPin 18

void setup() {
  Serial.begin(9600);
  pinMode(RainSensorPin, INPUT);
  pinMode(GreenLEDPin, OUTPUT);
  pinMode(BlueLEDPin, OUTPUT);
  pinMode(RedLEDPin, OUTPUT);
}

void loop() {
  int rainValue = analogRead(RainSensorPin);
  if(rainValue >= 3000){
    Serial.printf("Empty (%d)\n", rainValue);
    digitalWrite(RedLEDPin, HIGH);
    digitalWrite(GreenLEDPin, LOW);
    digitalWrite(BlueLEDPin, LOW);
  }
  else if(rainValue < 3000 && rainValue > 1500){
    Serial.printf("Stable (%d)\n", rainValue);
    digitalWrite(RedLEDPin, LOW);
    digitalWrite(GreenLEDPin, HIGH);
    digitalWrite(BlueLEDPin, LOW);
  }
  else if(rainValue <= 1500){
    Serial.printf("Full (%d)\n", rainValue);
    digitalWrite(RedLEDPin, HIGH);
    digitalWrite(GreenLEDPin, LOW);
    digitalWrite(BlueLEDPin, HIGH);
  }
  delay(1000);
}
