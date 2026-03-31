#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RadioLib.h>
#include <Wire.h>

#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define BAND 915.0 // RadioLib uses MHz floats (868.0 or 915.0)

// Heltec ESP32 V2 LoRa Pins
#define LORA_CS 18
#define LORA_DIO0 26
#define LORA_RST 14
#define LORA_DIO1 33

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

SX1276 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, LORA_DIO1);

byte receiverAddress = 0xCC;
byte senderAddress   = 0xBB;
bool sender = true;

long ignoredMessagesCount = 0;
long lastSendTime = 0;
byte messageNum = 0;

volatile bool receivedFlag = false;
void setFlag() {
  receivedFlag = true;
}

void setup() {
  Serial.begin(115200);

  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();

  printToScreen("Hello!");
  delay(500);

  // begin(Frequency, Bandwidth, SpreadingFactor, CodingRate, SyncWord, OutputPower, PreambleLength, Gain)
  int state = radio.begin(BAND, 125.0, 9, 5, 0x12, 14, 8, 0);

  if (state != RADIOLIB_ERR_NONE) {
    printToScreen("Radio init failed: " + String(state));
    while (true);
  }

  if (!sender) {
    radio.setPacketReceivedAction(setFlag);
    radio.startReceive();
  }
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
    if (receivedFlag) {
      receivedFlag = false;
      onReceive();
    }
  }
}

void printToScreen(String s) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.println(s);
  display.display();
}

void sendMessage(String message) {
  int len = 4 + message.length();
  byte payload[len];

  payload[0] = receiverAddress;
  payload[1] = senderAddress;
  payload[2] = messageNum;
  payload[3] = message.length();

  for(int i = 0; i < message.length(); i++) {
    payload[4 + i] = message[i];
  }

  radio.transmit(payload, len);
  messageNum += 1;
}

byte lastRecipient = 0;
byte lastSender = 0;
byte lastIncomingMessageNum = 0;
byte lastIncomingMessageLength = 0;
String lastIncomingMessage = "";
float lastRssi = 0;
float lastSnr = 0;

void onReceive() {
  int packetSize = radio.getPacketLength();
  if (packetSize == 0) {
    radio.startReceive();
    return;
  }

  byte payload[packetSize];
  int state = radio.readData(payload, packetSize);

  if (state == RADIOLIB_ERR_NONE && packetSize >= 4) {
    byte recipient = payload[0];
    byte senderId = payload[1];
    byte incomingMessageNum = payload[2];
    byte incomingMessageLength = payload[3];

    String incomingMessage = "";
    for (int i = 4; i < packetSize; i++) {
      incomingMessage += (char)payload[i];
    }

    if (incomingMessageLength != incomingMessage.length()) {
      printToScreen("error: invalid message length");
      radio.startReceive();
      return;
    }

    if (recipient == receiverAddress || recipient == 0xFF) {
      lastRecipient = recipient;
      lastSender = senderId;
      lastIncomingMessageNum = incomingMessageNum;
      lastIncomingMessageLength = incomingMessageLength;
      lastIncomingMessage = incomingMessage;
      lastRssi = radio.getRSSI();
      lastSnr = radio.getSNR();
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

  radio.startReceive();
}