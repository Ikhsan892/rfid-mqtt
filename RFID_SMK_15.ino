#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <PubSubClient.h>
//#include <LiquidCrystal_I2C.h>

#define RST_PIN         22          // ESP32
#define SS_PIN          21          // ESP32
#define BUZZ_PIN        2           // ESP32
#define GATE_PIN        2           // ESP32
//#define RST_PIN         D0        // ESP8266
//#define SS_PIN          D4        // ESP8266
//#define BUZZ_PIN        D8        // ESP8266
//#define GATE_PIN        D3        // ESP8266

//LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

char* ssid     = "FlowRezz"; // diganti SSID Wifi
const char* password = "Xnord3321"; // pw WiFi
const char* hostMqtt = "192.168.100.72"; // host mqtt
const int port = 1883; // port mqtt
const char* clientId = "ESP8266Client"; // client ID untuk MQTT
const char* topic = "IoT_command"; // topic

bool isRegistered = false;

WiFiClient wifiClient;
uint64_t openGateMillis = 0;
PubSubClient client(wifiClient);

void setup()
{
  pinMode(GATE_PIN, OUTPUT);
  pinMode(BUZZ_PIN, OUTPUT);
  digitalWrite(GATE_PIN, LOW);
  digitalWrite(BUZZ_PIN, LOW);


  Serial.begin(115200);
  delay(10);

//  lcd.init(); // Init with pin default ESP8266 or ARDUINO
//  lcd.backlight();
//  LcdClearAndPrint("Loading");


  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  delay(4);       // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details

  Serial.println(F("System Started and ready"));

  client.setServer(hostMqtt, port);
  client.setCallback(callback);
//  LcdClearAndPrint("Ready");
  delay(2000);
}

void callback(char* topic, byte *payload, unsigned int length) {
  String messageTemp = "";

  for (int i = 0; i < length; i++ ) {
    messageTemp += (char)payload[i];
  }

  Serial.println("-------new message from broker-----");
  Serial.print("channel:");
  Serial.println(topic);
  Serial.print("data:");
  Serial.print(messageTemp);
  Serial.println();


  if (messageTemp == "OPEN_GATE") {
    OpenGate();
  }

  if (messageTemp == "CLOSE_GATE") {
    CloseGate();
  }

  if (messageTemp == "CONNECTION_TEST") {
    Beep();
  }

  if (messageTemp == "LED_HIGH") {
    digitalWrite(2, HIGH);
  }

  if (messageTemp == "LED_LOW") {
    digitalWrite(2, LOW);
  }

}

void loop()
{ 
  if (!client.connected()) {
    reconnect();
  }
  client.loop();


  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
      if (openGateMillis > 0 && openGateMillis < millis())
      {
        CloseGate();
      }
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Dump debug info about the card; PICC_HaltA() is automatically called
  Serial.print(F("Card UID:"));
  String hex = hex_uid(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println(hex);
  int arrLen = hex.length() + 1;
  char charMsg[arrLen];
  hex.toCharArray(charMsg, sizeof(charMsg));


  Beep();
//  LcdClearAndPrint("Please wait...");
  client.publish("CARD_TAPPED", charMsg);



  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();



}

void OpenGate()
{
  openGateMillis = millis() + 5000;
  digitalWrite(GATE_PIN, HIGH);
  Beep();
  delay(100);
  Beep();
}

void Beep()
{
  for (int i = 0; i < 1000; i++)
  {
    analogWrite(BUZZ_PIN, i);
    delayMicroseconds(50);
  }
  digitalWrite(BUZZ_PIN, LOW);
}


void Beep2()
{
  tone(BUZZ_PIN, 1000, 30);
  delay(300);
  digitalWrite(BUZZ_PIN, LOW);
}



void CloseGate()
{
  openGateMillis = 0;
  digitalWrite(GATE_PIN, LOW);
  Beep2();
//  LcdClearAndPrint("Ready");
}


String hex_uid(byte *buffer, byte bufferSize) {
  String tagContent = "";
  for (byte i = 0; i < bufferSize; i++) {
    tagContent.concat(String(buffer[i] < 0x10 ? " 0" : " "));
    tagContent.concat(String(buffer[i], HEX));
  }

  return tagContent;
}

//void LcdClearAndPrint(String text)
//{
//  lcd.clear();
//  lcd.setCursor(0, 0);
//  lcd.print(text);
//}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientId)) {
      Serial.println("connected");
      client.subscribe(topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
