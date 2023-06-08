#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <AccelStepper.h>
#include <Ethernet.h>

#define RFID_VALID 1
#define RFID_INVALID 0
#define RFID_FAILED -1

// If using the breakout or shield with I2C, define just the pins connected
// to the IRQ and reset lines.  Use the values below (2, 3) for the shield!
#define PN532_IRQ   (13)
#define PN532_RESET (6)  // Not connected by default on the NFC Shield

#define PIN_STEP 2
#define PIN_DIR 3

#define SER 7
#define RCLK 8
#define SRCLK 9

#define TASTER_ENDSCHALTER A0
#define TASTER_REED A1
#define TASTER_ABSCHLIESSEN A2
#define TASTER_TERASSE A3

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
#define STEPPER_SPEED 300
#define STEPPER_ACCEL 1000
#define STEPS_TO_OPEN 500
#define STEPS_TO_CLOSE 500
#define STEPS_MAX 2000

AccelStepper stepper(1, PIN_STEP, PIN_DIR);

String lastUid;
int retryCount = 0;

char check(String rfid);
void updateSR();
void closeDoor();
void openDoor(boolean forceOpening);

// setup RFID
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

void setup(void) {

  // PinModes
  pinMode(SER, OUTPUT);
  pinMode(RCLK, OUTPUT);
  pinMode(SRCLK, OUTPUT);
  pinMode(TASTER_ENDSCHALTER, INPUT_PULLUP);
  pinMode(TASTER_REED, INPUT_PULLUP);
  pinMode(TASTER_ABSCHLIESSEN, INPUT_PULLUP);

  digitalWrite(SER, LOW);
  digitalWrite(RCLK, LOW);
  digitalWrite(SRCLK, LOW);

  srYellow = 1;
  updateSR();

  // Stepper
  stepper.setMaxSpeed(STEPPER_SPEED);
  stepper.setAcceleration(STEPPER_ACCEL);

  Serial.begin(115200);
  while (!Serial) delay(10); // for Leonardo/Micro/Zero
  
  Serial.println("Hello!");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

  Serial.println("Waiting for an ISO14443A Card ...");

   if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }

  srYellow = 0;
  updateSR();

  Serial.println("Ready!");
}

int maintainCounter = 0;
void loop(void) {
  maintainCounter++;
  if (maintainCounter >= 6000) {
    maintainCounter = 0;
    Ethernet.maintain();
  }
  // Look for new cards 
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    // Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");

  String _uid = "";
  for (byte i = 0; i < uidLength; i++) {
    if (uid[i] < 16){
      _uid = _uid + "0";
    }
    _uid = _uid + String(uid[i], 16);
    if (i < uidLength - 1 ) {
      _uid = _uid + ":";
    }
  
  }
  Serial.println("+++++++++++++++++++++");
  Serial.println(_uid);
  Serial.println("+++++++++++++++++++++");
  //Something is wrong with the check function
  char result = check(_uid);

  closeDoor();

  srYellow = 0;
  updateSR();
  if (result == RFID_VALID) {
    if (lastUid.equals(_uid)) {
      retryCount++;
    }
    if (retryCount == 2) {
      Serial.println("I will open the door");
      openDoor(true);
    } else {
      Serial.println("Card has not been recognized");
      openDoor(false);
    }
  } else if (result == RFID_INVALID){
    retryCount = 0;
    lastUid = "";
    srRed = 1;
    updateSR();
    delay(2000);
    srRed = 0;
    updateSR();
  } else {
    // ...
  }
  lastUid = _uid;
  //mfrc522.PCD_Init(); 


  
  }

    
}

void openDoor(boolean forceOpening) {
  if (digitalRead(TASTER_ENDSCHALTER) == 0 || forceOpening){
    srMotorEnable = 1;
    srGreen = 1;
    updateSR();

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
    updateSR();
  }
  srGreen = 1;
  srSummer = 1;
  updateSR();
  delay(2000);
  srGreen = 0;
  srSummer = 0;
  updateSR();
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
  updateSR();

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
  updateSR();
  delay(500);
  srGreen = 1;
  srYellow = 1;
  srRed = 1;
  updateSR();
  delay(500);
  srGreen = 0;
  srYellow = 0;
  srRed = 0;
  updateSR();
  delay(500);
  srGreen = 1;
  srYellow = 1;
  srRed = 1;
  updateSR();
  delay(500);
  srGreen = 0;
  srYellow = 0;
  srRed = 0;
  updateSR();
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

    // here for testing
    return RFID_VALID;

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