/*
 * Uses a MAX4466 microphone to take samples
 * The mic must use a seperate power supply (3.3v or 5v) as the ESP32 has too much noise on power lines
 * 
 * 
 */

#include <WiFi.h>
//note I included <adc.h> then deleted it. Not sure if it's needed
#define BUFFERSIZE 1024

const char* ssid = "SKYKVJMF";
const char* password = "g5uqt7uCS3Xn";
const uint16_t port = 8090;
const char * host = "192.168.0.16";
unsigned long previousMillis = 0;
unsigned long reconnectInterval = 3000;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
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
  uint8_t sampleArray[1024] = {0};
  for (uint16_t count = 0; count < sizeof(sampleArray); count++){
      uint16_t sample = analogRead(34);
      uint8_t resizedSample = map(sample, 0, 4096, 0, 255);
//            Serial.println(resizedSample);
      sampleArray[count] = resizedSample;
  }
  WiFiClient client;
 
    if (!client.connect(host, port)) {
 
        Serial.println("Connection to host failed");
 
        delay(1000);
        return;
    }
 
    Serial.println("Connected to server successful!");
//        client.print("Hello from ESP32!");
    client.write(sampleArray, sizeof(sampleArray));
    client.stop();
//  delay(100);
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