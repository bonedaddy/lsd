#include <LoRa.h>


void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Sender");

  if (!LoRa.begin(8681E5)) {

    Serial.println("Starting LoRa failed!");
    while (1);
  }
}



void loop() {
  // send packet
  LoRa.beginPacket();
  LoRa.print("hello world");
  //LoRa.print(counter);
  LoRa.endPacket();
}

