#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include <AccelStepper.h>
#include <Ethernet.h>


#define RFID_VALID 1
#define RFID_INVALID 0
#define RFID_FAILED -1

/* 
 *  Pin definitions
 *  
  SPI:
  SCK     = 13
  MISO    = 12
  MOSI    = 11
  SS-Etha = 10
  SS-SD   = 4
  SS-RFID = 6
*/

#define PIN_STEP 2
#define PIN_DIR 3

#define SER 7
#define RCLK 8
#define SRCLK 9

#define TASTER_ENDSCHALTER A0
#define TASTER_REED A1
#define TASTER_ABSCHLIESSEN A2
#define TASTER_TERASSE A3

//RFID
//#define PN532_SS   6
//#define PN532_MOSI 11
//#define PN532_MISO 12
//#define PN532_SCK  13

//I2C RFID
#define PN532_IRQ   13
#define PN532_RESET 6  

//ETH Shield
#define ETH_SS   10

// Schiftregister
#define SR_RED 0x1
#define SR_YELLOW 0x2
#define SR_GREEN 0x4
#define SR_SUMMER 0x8
#define SR_MOTOR_ENABLE 0x10
#define SR_WARNING_SUMMER 0x20
#define SR_WARNING_LED 0x40

char srOutput = 0;
char srRed = 0;
char srYellow = 0;
char srGreen = 0;
char srSummer = 0;
char srMotorEnable = 0;

// Network
byte mac[] = {0x74,0xD0,0x2B,0x19,0x0E,0x48};
IPAddress ip(192, 168, 178, 177); //  static IP address to use if the DHCP fails to assign
// String serverIP = "192.168.178.45";  // IP of Server(Raspberry Pi)
// IPAddress server(192,168,178,54);  // IP of Server(Raspberry Pi)
String serverIP = "192.168.178.79";  // IP of Server(Raspberry Pi)
IPAddress server(192,168,178,79);  // IP of Server(Raspberry Pi)
EthernetClient client;

// Stepper
#define STEPPER_SPEED 1200
#define STEPPER_ACCEL 2000
#define STEPS_TO_OPEN 500
#define STEPS_TO_CLOSE 500
#define STEPS_MAX 2000

AccelStepper stepper(1, PIN_STEP, PIN_DIR);

String lastUid;
int retryCount = 0;

// RFID
//Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

//void updateSR();
//void closeDoor();
//void openDoor(boolean forceOpening);

Adafruit_PN532 nfc(PN532_IRQ,PN532_RESET);

void setup() {

  // PinModes
  //pinMode(SER, OUTPUT);
  //pinMode(RCLK, OUTPUT);
  //pinMode(SRCLK, OUTPUT);
  //pinMode(TASTER_ENDSCHALTER, INPUT_PULLUP);
  //pinMode(TASTER_REED, INPUT_PULLUP);
  //pinMode(TASTER_ABSCHLIESSEN, INPUT_PULLUP);

  //srYellow = 1;
  //updateSR();

  // Stepper
  //stepper.setMaxSpeed(STEPPER_SPEED);
  //stepper.setAcceleration(STEPPER_ACCEL);

  Serial.begin(115200);
  //while (!Serial) delay(10);

  // RFID
  Serial.println("Init RFID");
  //mfrc522.PCD_Init();

  nfc.begin();
  Serial.print("Found chip PN5");
  
  uint32_t versiondata = nfc.getFirmwareVersion();
  while (! versiondata) {
    Serial.println("Didn't find PN53x board");
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);

  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  nfc.setPassiveActivationRetries(0xFF);

  Serial.println("Waiting for an ISO14443A card");

  // Ethernet
  Serial.println("Init Ethernet");
  // start the Ethernet connection:
  
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  

  srYellow = 0;
  //updateSR();

  Serial.println("Ready!");
}

char check(String rfid) {
  Serial.println("connecting...");
  // if you get a connection, report back via serial:
  if (client.connect(server, 80)) {
  //if (client.connect(server, 6543)) {
    Serial.println("connected");
    // Make a HTTP request:
    // client.println("GET /lock.php?RFID=" + rfid + " HTTP/1.1");
    client.println("GET /check_door_key/21042017freilab1337fooboardasgeht111/holz/" + rfid + " HTTP/1.1");
    client.println("Host: " + serverIP);
    client.println("Connection: close");
    client.println();

    // read response
    int numCr = 0;
    String message = "";
    while(client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') numCr++;
        Serial.print(c);
        if (numCr > 6) {
          message += c;  
        }
      }
    }
    Serial.println("disconnecting.");
    client.stop();
    message.trim();
    Serial.println("Response: " + message);
    if (message.equals("<html><head></head><body>true</body></html>")) {
      return RFID_VALID;
    } else {
      return RFID_INVALID;
    }
    
  } else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
    return RFID_FAILED;
  }
}

int maintainCounter = 0;

void loop() {
  maintainCounter++;
  if (maintainCounter >= 6000) {
    maintainCounter = 0;
    Ethernet.maintain();
  }
  // Look for new cards
  Serial.println("main");
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };	// Buffer to store the returned UID
  uint8_t uidLength;				// Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  Serial.println("main1");
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  Serial.println("main2");
  if (success) {
    Serial.println("Found a card!");
    Serial.print("UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i=0; i < uidLength; i++)
    {
      Serial.print(" 0x");Serial.print(uid[i], HEX);
    }
    Serial.println("");
	// Wait 1 second before continuing
	delay(1000);
  }
  else
  {
    // PN532 probably timed out waiting for a card
    Serial.println("Timed out waiting for a card");
  }

  //if ( ! success) {
  //  if(digitalRead(TASTER_ABSCHLIESSEN) == 0) {
  //    closeDoor();
  //  }
  //  delay(50);
  //  return;
  //}
  //// Select one of the cards
  //
  //if ( ! success) {
  //  delay(50);
  //  return;
  //}
  //srYellow = 1;
  //updateSR();
  //// Show some details of the PICC (that is: the tag/card)
  //Serial.print(F("Card UID:"));
  ////String uid = "";
  //for (uint8_t i=0; i < uidLength; i++)
  //  {
  //    Serial.print(" 0x");Serial.print(uid[i], HEX);
  //  }
  //for (byte i = 0; i < nfc._uid .uid.size; i++) {
  //  if (mfrc522.uid.uidByte[i] < 16){
  //    uid = uid + "0";
  //  }
  //  uid = uid + String(mfrc522.uid.uidByte[i], 16);
  //  if (i < mfrc522.uid.size - 1 ) {
  //    uid = uid + ":";
  //  }
  //}
  
  
  //Serial.println(uid);
  //char result = check(uid);
  //srYellow = 0;
  //updateSR();
  //if (result == RFID_VALID) {
  //  if (lastUid.equals(uid)) {
  //    retryCount++;
  //  }
  //  if (retryCount == 2) {
  //    openDoor(true);
  //  } else {
  //    openDoor(false);
  //  }
  //} else if (result == RFID_INVALID){
  //  retryCount = 0;
  //  lastUid = "";
  //  srRed = 1;
  //  updateSR();
  //  delay(2000);
  //  srRed = 0;
  //  updateSR();
  //} else {
  //  // ...
  //}
  //lastUid = uid;
  //mfrc522.PCD_Init(); 
}


void openDoor(boolean forceOpening) {
  if (digitalRead(TASTER_ENDSCHALTER) == 0 || forceOpening){
    srMotorEnable = 1;
    srGreen = 1;
    //updateSR();

    stepper.setSpeed(STEPPER_SPEED);
    while(digitalRead(TASTER_ENDSCHALTER) == 0){
      stepper.runSpeed();
    }
    long pos = stepper.currentPosition();

    while(stepper.currentPosition() - pos < STEPS_TO_OPEN){
      stepper.runSpeed();
    }
    stepper.setSpeed(0);
          
    srMotorEnable = 0;
    //updateSR();
  }
  srGreen = 1;
  srSummer = 1;
  //updateSR();
  delay(2000);
  srGreen = 0;
  srSummer = 0;
  //updateSR();
}

void closeDoor() {
  lastUid = "";
  retryCount = 0;
  while (digitalRead(TASTER_REED));
  delay(2000);
  
  srMotorEnable = 1;
  srGreen = 1;
  srYellow = 1;
  srRed = 1;
  //updateSR();

  long posStart = stepper.currentPosition();
  stepper.setSpeed(-STEPPER_SPEED);
  while(digitalRead(TASTER_ENDSCHALTER) == 1 && posStart - stepper.currentPosition() < STEPS_MAX){
    stepper.runSpeed();
  }

  long pos = stepper.currentPosition();

  while(pos - stepper.currentPosition() < STEPS_TO_CLOSE){
    stepper.runSpeed();
  }

  srMotorEnable = 0;
  srGreen = 0;
  srYellow = 0;
  srRed = 0;
  //updateSR();
  delay(500);
  srGreen = 1;
  srYellow = 1;
  srRed = 1;
  //updateSR();
  delay(500);
  srGreen = 0;
  srYellow = 0;
  srRed = 0;
  //updateSR();
  delay(500);
  srGreen = 1;
  srYellow = 1;
  srRed = 1;
  //updateSR();
  delay(500);
  srGreen = 0;
  srYellow = 0;
  srRed = 0;
  //updateSR();
}

void updateSR() {
  digitalWrite(SRCLK, LOW);
  digitalWrite(RCLK, LOW);
  
  if (srMotorEnable == 1){
    digitalWrite(SER, LOW);
  } else {
    digitalWrite(SER, HIGH);
  }
  digitalWrite(SRCLK, HIGH);
  digitalWrite(SRCLK, LOW);

  if (srSummer == 0){
    digitalWrite(SER, HIGH);
  } else {
    digitalWrite(SER, LOW);
  }
  digitalWrite(SRCLK, HIGH);
  digitalWrite(SRCLK, LOW);

  if (srGreen == 0){
    digitalWrite(SER, HIGH);
  } else {
    digitalWrite(SER, LOW);
  }
  digitalWrite(SRCLK, HIGH);
  digitalWrite(SRCLK, LOW);

  if (srYellow == 0){
    digitalWrite(SER, HIGH);
  } else {
    digitalWrite(SER, LOW);
  }
  digitalWrite(SRCLK, HIGH);
  digitalWrite(SRCLK, LOW);

  if (srRed == 0){
    digitalWrite(SER, HIGH);
  } else {
    digitalWrite(SER, LOW);
  }
  digitalWrite(SRCLK, HIGH);
  digitalWrite(SRCLK, LOW);

  digitalWrite(RCLK, HIGH);
  digitalWrite(RCLK, LOW);
}
