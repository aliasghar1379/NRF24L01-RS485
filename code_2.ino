#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <SoftwareSerial.h>

#define RS485_RX 2
#define RS485_TX 3
#define RS485_DE 4

#define PACKET_SIZE 33

SoftwareSerial RS485Serial(RS485_RX, RS485_TX);

RF24 radio(9, 10);
const byte addresses[][6] = { "00001", "00002" };

char encodeChar(char c) {
  // Encode the character in some way (e.g., using a simple shift cipher)
  return c + 1;
}

char decodeChar(char c) {
  // Decode the character in the same way it was encoded
  return c - 1;
}

void encodePacket(char* packet, int len) {
  for (int i = 0; i < len; i++) {
    packet[i] = encodeChar(packet[i]);
  }
}

void decodePacket(char* packet, int len) {
  for (int i = 0; i < len; i++) {
    packet[i] = decodeChar(packet[i]);
  }
}

void setup() {
  // Serial.begin(9600);
  RS485Serial.begin(9600);
  SPI.setClockDivider(SPI_CLOCK_DIV2); 
  pinMode(RS485_DE, OUTPUT);
  digitalWrite(RS485_DE, LOW);
  radio.begin();
  radio.openWritingPipe(addresses[1]);
  radio.openReadingPipe(1, addresses[1]);
  radio.setChannel(100);
  radio.setPALevel(RF24_PA_MAX);
  radio.setAutoAck(true);
  radio.setDataRate(RF24_2MBPS);
  radio.startListening();
}

void loop() {
  char receivedPacket[PACKET_SIZE];
  int receivedIndex = 0;

  if (radio.available()) {
    radio.read(receivedPacket, sizeof(receivedPacket));
    receivedIndex = receivedPacket[0]; // Read the length of the packet

    if (receivedIndex > 0) {
      decodePacket(&receivedPacket[1], receivedIndex); // Decode the received packet
      digitalWrite(RS485_DE, HIGH);
      RS485Serial.write(&receivedPacket[1], receivedIndex);  // Write the entire buffer at once
      digitalWrite(RS485_DE, LOW);
    }
  }

  char sendPacket[PACKET_SIZE] = {};
  int sendIndex = 0;

  while (RS485Serial.available()) {
    if (sendIndex < PACKET_SIZE - 1) {  // Buffer the data to be transmitted
      sendPacket[sendIndex + 1] = RS485Serial.read(); // Add 1 to skip the first byte, which will hold the length of the packet
      sendIndex++;
    }
  }

  if (sendIndex > 0) {
    encodePacket(&sendPacket[1], sendIndex); // Encode the data to be sent
    sendPacket[0] = sendIndex; // Set the first byte to indicate the length of the packet
    radio.stopListening();
    radio.write(sendPacket, sendIndex + 1);  // Write the entire buffer at once (+1 to include the length byte)
    radio.startListening();
  }
}
