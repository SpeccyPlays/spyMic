#include <Arduino.h>
/*
 * Uses a MAX4466 microphone to take samples
 * The mic must use a seperate power supply (3.3v or 5v) as the ESP32 has too much noise on power lines
 * Data Received by a C++ program which saves the data in a file
 * File can be opened in Audacity as unsigned 8 bit PCM, endianess makes no difference, and played at a speed of 11,025hz (roughly)
 * 
 */

#include <WiFi.h>
//note I included <adc.h> then deleted it. Not sure if it's needed
#define LEDPIN 2
#define BUFFERSIZE 1024

const char* ssid = "YOUR SSID";
const char* password = "YOUR WIFI PASSWORD";
const uint16_t port = 8090;
const char * host = "YOUR SERVER IP ADDRESS";
unsigned long previousMillis = 0;
unsigned long reconnectInterval = 3000;
int inputPin = 13;
void reconnectToAP();
uint8_t sampleArray[BUFFERSIZE];

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(LEDPIN, OUTPUT);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Attempting to connect to wifi...");
  }
  Serial.print("WiFi connected with IP: ");
  Serial.println(WiFi.localIP());
  analogReadResolution(12);
  adcAttachPin(34);
  delay(500);
}

void loop() {
  // put your main code here, to run repeatedly:
  for (uint16_t count = 0; count < sizeof(sampleArray); count++){
      uint16_t sample = analogRead(34);
      uint8_t resizedSample = map(sample, 0, 4096, 0, 255);
      sampleArray[count] = resizedSample;    
  }
  WiFiClient client;
  if (!client.connect(host, port)) {
    Serial.println("Connection to host failed");
    digitalWrite(LEDPIN, HIGH);
    delay(1000);
    digitalWrite(LEDPIN, LOW);
    return;
  }
  else {
      Serial.println("Connected to server successful!");
      client.write(sampleArray, sizeof(sampleArray));
      client.stop();
  }
  reconnectToAP();
}

void reconnectToAP(){
  /*
   * If the connection to wifi disconnects then try to reconnect after the reconnectInterval has passed
   */
  unsigned long currentMillis = millis();
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= reconnectInterval)){
    Serial.println("Attempting reconnect...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  }
}