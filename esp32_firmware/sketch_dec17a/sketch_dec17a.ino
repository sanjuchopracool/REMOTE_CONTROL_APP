/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE" 
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second. 
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ESP32Servo.h> 


BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
float txValue = 0;
BLEServer *pServer = nullptr;
char buffer[32] = {};

int16_t roll = 0;
int16_t pitch = 0;
int16_t throttle = 0;
int16_t yaw = 0;

// servo 
Servo servo_steering;
Servo servo_throttle;
constexpr int steering_pin = 8;
constexpr int throttle_pin = 9;

bool previous_connected = false;
//std::string rxValue; // Could also make this a global var to access it in loop()

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
       Serial.println("*********Connected*********");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("*********disconnected*********");
      pServer->startAdvertising();
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String rxValue = pCharacteristic->getValue();

      if (rxValue.length() == 8) {
            // A E T R
    // A -> ROLL
    // E -> PITCH
    // T -> Throttle
    // R -> Yaw
     roll = (rxValue[1] << 8) +  rxValue[0];
     pitch = (rxValue[3] << 8) +  rxValue[2];
     throttle = (rxValue[5] << 8) +  rxValue[4];
     yaw = (rxValue[7] << 8) +  rxValue[6];

     servo_throttle.writeMicroseconds(1500 + pitch/20);
     servo_steering.writeMicroseconds(1500 + yaw/20);
    //  sprintf(buffer, "%d %d %d %d", roll, pitch, throttle, yaw);
    //  Serial.println(buffer);
      }
    }
};

void setup() {
  Serial.begin(115200);
  servo_steering.attach(steering_pin, 1000, 2000);
  servo_throttle.attach(throttle_pin, 1000, 2000);

  // Create the BLE Device
  BLEDevice::init("ESP32 UART Test"); // Give it a name

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
                      
  pCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");

  servo_throttle.writeMicroseconds(1500);
  servo_steering.writeMicroseconds(1500);
}

void loop() {
  if (deviceConnected) {
//     // Fabricate some arbitrary junk for now...
//     txValue = 1.23; //analogRead(readPin) / 3.456; // This could be an actual sensor reading!

//     // Let's convert the value to a char array:
//     char txString[8]; // make sure this is big enuffz
//     dtostrf(txValue, 1, 2, txString); // float_val, min_width, digits_after_decimal, char_buffer
    
// //    pCharacteristic->setValue(&txValue, 1); // To send the integer value
// //    pCharacteristic->setValue("Hello!"); // Sending a test message
//     pCharacteristic->setValue(txString);
    
//     pCharacteristic->notify(); // Send the value to the app!
  } else {
    if (previous_connected == true) {
        servo_throttle.writeMicroseconds(1500);
    }
  }

  previous_connected = deviceConnected;

}