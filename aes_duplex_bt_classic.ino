#include "heltec.h"
#include "Cipher.h"           // code for aes encryption and decryption functions
#include "mbedtls/aes.h"      // hardware accelerated aes provided by https://tls.mbed.org/ (apache license)
#include "BluetoothSerial.h"  //fork with bluetooth support for serial communication

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif



#define BAND    433E6         // regional bands


BluetoothSerial SerialBT;

Cipher * cipher = new Cipher();

byte msgCount = 0;            // count of outgoing messages
int interval = 2000;          // interval between sends
long lastSendTime = 0;        // time of last packet send
String message = "";
void setup()
{
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.Heltec.Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  LoRa.setSyncWord(0xF3);           // ranges from 0-0xFF, default 0x34
  Serial.begin(115200);
  SerialBT.begin("esp32 duplex0");
  Serial.println("Bluetooth started!");
}

void loop()
{
  char * key = "abcdefghijklmnop"; // encryption key
  cipher->setKey(key);

  if (millis() - lastSendTime > interval)
  {
    if (SerialBT.available() > 0) {
      // read the incoming string:
      message = SerialBT.readString();
    }
    // message += msgCount;
    String cipherString = cipher->encryptString(message);
    sendMessage(cipherString);
    SerialBT.println("Sending: " + cipherString);
    lastSendTime = millis();            // timestamp the message
    interval = 1000;     // 1 second
    msgCount++;
  }

  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());
}

void sendMessage(String outgoing)
{
  LoRa.beginPacket();                   // start packet
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}

void onReceive(int packetSize)
{
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  String incoming = "";
  String cipherString1 = "";
  while (LoRa.available())
  {
    cipherString1 = LoRa.readString();
  }
  char * key1 = "abcdefghijklmnop"; //decryption key
  cipher->setKey(key1);
  incoming = cipher->decryptString(cipherString1);

  SerialBT.println("Incoming: " + incoming);
  //Serial.println("RSSI: " + String(LoRa.packetRssi()));
  //Serial.println("Snr: " + String(LoRa.packetSnr()));
  SerialBT.println();
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->drawString(5, 5, String(incoming));
  Heltec.display->display();
}
