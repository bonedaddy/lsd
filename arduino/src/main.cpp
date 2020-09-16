#include <LoRa.h>
/*!
#define SF 7
#define SYNC_WORD 0x34

void onReceive(int packetSize);
void onTxDone();

bool first_run = true;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Sender");

  if (!LoRa.begin(8681E5)) {

    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.setSpreadingFactor(SF);
  LoRa.setSyncWord(SYNC_WORD);
  LoRa.onReceive(onReceive);
  LoRa.onTxDone(onTxDone);
  LoRa.receive();
}


void onTxDone() { LoRa.receive(); }

// callback function whenever we receive a LoRa packet
void onReceive(int packetSize) {
  if (packetSize) {
    char buffer[255];
    int num = LoRa.readBytes(buffer, 255);
    Serial.write(buffer, num);
    Serial.flush();
    if (LoRa.beginPacket()) {
      LoRa.print(buffer);
      LoRa.endPacket();
      LoRa.receive();
    }
  }
}


void loop() {

}
*/

#include <LoRa.h>

int counter = 0;

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
  char buffer[32];

  sprintf(buffer, "hello world: %i", counter);

  LoRa.print(buffer);

  LoRa.endPacket();

  counter += 1;
}


