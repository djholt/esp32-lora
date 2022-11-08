#include "heltec.h"

#define BAND 868E6 // 868E6 or 915E6

byte receiverAddress = 0xCC;
byte senderAddress   = 0xBB;
bool sender = true;

long ignoredMessagesCount = 0;
long lastSendTime = 0;
byte messageNum = 0;

void setup() {
  Heltec.begin(true /*display*/, true /*LoRa*/, true /*Serial*/, true /*PABOOST*/, BAND);
}

void loop() {
  if (sender) {
    if (millis() - lastSendTime > 1000) {
      String message = "Hello world.";
      sendMessage(message);
      String line1 = "From: 0x" + String(senderAddress, HEX) + " To: 0x" + String(receiverAddress, HEX);
      String line2 = "Sending: " + message;
      String line3 = "Message ID: " + String(messageNum);
      printToScreen(line1 + "\n" + line2 + "\n" + line3);
      lastSendTime = millis();
    }
  } else {
    onReceive(LoRa.parsePacket());
  }
}

void printToScreen(String s) {
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, s);
  Heltec.display->display();
}

void sendMessage(String message) {
  LoRa.beginPacket();
  LoRa.write(receiverAddress);
  LoRa.write(senderAddress);
  LoRa.write(messageNum);
  LoRa.write(message.length());
  LoRa.print(message);
  LoRa.endPacket();
  messageNum += 1;
}

byte lastRecipient = 0;
byte lastSender = 0;
byte lastIncomingMessageNum = 0;
byte lastIncomingMessageLength = 0;
String lastIncomingMessage = "";
int lastRssi = 0;
int lastSnr = 0;

void onReceive(int packetSize) {
  if (packetSize == 0) {
    return;
  }

  byte recipient = LoRa.read();
  byte sender = LoRa.read();
  byte incomingMessageNum = LoRa.read();
  byte incomingMessageLength = LoRa.read();

  String incomingMessage = "";
  while (LoRa.available()) {
    incomingMessage += (char) LoRa.read();
  }

  if (incomingMessageLength != incomingMessage.length()) {
    printToScreen("error: invalid message length");
    return;
  }

  if (recipient == receiverAddress || recipient == 0xFF) {
    lastRecipient = recipient;
    lastSender = sender;
    lastIncomingMessageNum = incomingMessageNum;
    lastIncomingMessageLength = incomingMessageLength;
    lastIncomingMessage = incomingMessage;
    lastRssi = LoRa.packetRssi();
    lastSnr = LoRa.packetSnr();
  } else {
    ignoredMessagesCount += 1;
  }

  String line1 = "From: 0x" + String(lastSender, HEX) + " To: 0x" + String(lastRecipient, HEX);
  String line2 = "Received: " + lastIncomingMessage;
  String line3 = "Message ID: " + String(lastIncomingMessageNum);
  String line4 = "Ignored messages: " + String(ignoredMessagesCount);
  String line5 = "RSSI: " + String(lastRssi) + " SNR: " + String(lastSnr);
  printToScreen(line1 + "\n" + line2 + "\n" + line3 + "\n" + line4 + "\n" + line5);
}
