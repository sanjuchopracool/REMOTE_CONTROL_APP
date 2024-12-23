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

int16_t throttle = 0;
int16_t steering = 0;

enum Flags
{
  Invert_Throttle = 0x01,
  Invert_Steering = 0x02,
};

bool throttle_inverted = true;
bool steering_inverted = false;
int8_t throttle_front_percentage = 50;
int8_t throttle_back_percentage = 30;
int8_t steering_percentage = 100;

// servo
Servo servo_steering;
Servo servo_throttle;

constexpr int steering_pin = 8;
constexpr int throttle_pin = 9;
constexpr int buzzer_pin = 10;


unsigned long last_data_receieved_time = 0;
enum class BlinkStatus : uint8_t {
  OFF,
  ON,
  BlinkSlow,  // 0.5 Hz Hz
  BlinkMedium, // 1 Hz
  BlinkFast    // 2.5 Hz
};

BlinkStatus buzzer_status = BlinkStatus::BlinkSlow;

bool previous_connected = false;
bool config_received = false;
// std::string rxValue; // Could also make this a global var to access it in loop()

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


void update_pin_status(BlinkStatus status, int pin, unsigned long millis) {
  switch(status) {
    case BlinkStatus::OFF:
      digitalWrite(pin, LOW);
      return;
    case BlinkStatus::ON:
      digitalWrite(pin, HIGH);
      return;
    default:
      break;
  }

  // blink logic, positive low, negative high
  if (status == BlinkStatus::BlinkSlow) {
     digitalWrite(pin, (millis/1000)%2);
  } else if (status == BlinkStatus::BlinkMedium) {
    digitalWrite(pin, (millis/500)%2);
  }if (status == BlinkStatus::BlinkFast) {
    digitalWrite(pin, (millis/200)%2);
  }
}

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
    Serial.println("*********Connected*********");
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
    buzzer_status = BlinkStatus::BlinkSlow;
    Serial.println("*********disconnected*********");
    pServer->startAdvertising();
  }
};

class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    String rxValue = pCharacteristic->getValue();

    if (config_received && rxValue.length() == 4)
    {
      last_data_receieved_time = millis();
      buzzer_status = BlinkStatus::OFF;
      // Serial.println("Blink OFF");
      // int18_t throttle, steering
      throttle = (rxValue[1] << 8) + rxValue[0];
      steering = (rxValue[3] << 8) + rxValue[2];

    // 1500 for middle, , incoming value +- 10000 , value*500/10000 = value / 20
    // for percentage multi ply by it and devide by 100
    // invert
      int8_t throttle_dir = throttle_inverted ? -1 : 1;
      int8_t steering_dir = steering_inverted ? -1 : 1; 

      int th = throttle < 0 ? throttle*throttle_back_percentage : throttle*throttle_front_percentage;
      th = th*throttle_dir / 2000;
      int st = steering*steering_dir*steering_percentage / 2000;

      servo_throttle.writeMicroseconds(1500 + th);
      servo_steering.writeMicroseconds(1500 + st);
      // Serial.print("Th = ");
      // Serial.println(1500 + th);
      // Serial.print("St = ");
      // Serial.println(1500 + st);
    }
    else if (rxValue.length() == 5)
    {
      last_data_receieved_time = millis();
      // flags, steering, front, back, dummy
      uint8_t flags = rxValue[0];
      throttle_inverted = flags & Invert_Throttle;
      steering_inverted = flags & Invert_Steering;

      steering_percentage = rxValue[1];
      throttle_front_percentage = rxValue[2];
      throttle_back_percentage = rxValue[3];
      //  sprintf(buffer, "%d %d %d %d", roll, pitch, throttle, yaw);
       Serial.println("Config Receieved");

       if (!config_received) {
          buzzer_status = BlinkStatus::BlinkMedium;
          Serial.println("Medium Blink");
          config_received = true;
       }
    }
  }
};

void setup()
{
  Serial.begin(115200);
  servo_steering.attach(steering_pin, 1000, 2000);
  servo_throttle.attach(throttle_pin, 1000, 2000);
  pinMode(buzzer_pin, OUTPUT);

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
      BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");

  servo_throttle.writeMicroseconds(1500);
  servo_steering.writeMicroseconds(1500);
}

void loop()
{
  if (deviceConnected)
  {
    //     // Fabricate some arbitrary junk for now...
    //     txValue = 1.23; //analogRead(readPin) / 3.456; // This could be an actual sensor reading!

    //     // Let's convert the value to a char array:
    //     char txString[8]; // make sure this is big enuffz
    //     dtostrf(txValue, 1, 2, txString); // float_val, min_width, digits_after_decimal, char_buffer

    // //    pCharacteristic->setValue(&txValue, 1); // To send the integer value
    // //    pCharacteristic->setValue("Hello!"); // Sending a test message
    //     pCharacteristic->setValue(txString);

    //     pCharacteristic->notify(); // Send the value to the app!
  }
  else
  {
    if (previous_connected == true)
    {
      servo_throttle.writeMicroseconds(1500);
    }
  }

  previous_connected = deviceConnected;

  auto time_in_ms = millis();
  update_pin_status(buzzer_status, buzzer_pin, time_in_ms);

  if (deviceConnected && config_received) {
    //if we have not received in last one second

    // sometime last_time is recent then time_in_ms
    // becasue its updated from callback (perhaps using interrupt)
    const int diff = (time_in_ms - last_data_receieved_time);
    if (diff > 1000) {
      buzzer_status = BlinkStatus::BlinkFast;
    }
  }
}