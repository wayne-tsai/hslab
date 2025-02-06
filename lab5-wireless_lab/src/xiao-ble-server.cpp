#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <stdlib.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long previousMillis = 0;
const long interval = 1000;

// TODO: add new global variables for your sensor readings and processed data
const int trigPin = D0;
const int echoPin = D1;
const float soundSpeed = 0.034;

const int numReadings = 10;
int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average

// TODO: Change the UUID to your own (any specific one works, but make sure they're different from others'). You can generate one here: https://www.uuidgenerator.net/
#define SERVICE_UUID        "16001e21-1e24-47ff-a0e4-b41fbad50058"
#define CHARACTERISTIC_UUID "d609117c-7027-4e19-a8a9-737de429c210"

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};
float measureDistance() {
  // Clear the trigPin by setting it LOW
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Set the trigPin HIGH for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Read the echoPin, returns the sound wave travel time in microseconds
  long duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance
  float distance = duration * soundSpeed / 2;
  
  // Print the distance on the Serial Monitor
  return distance;
}
// TODO: add DSP algorithm functions here

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    // TODO: add codes for handling your sensor setup (pinMode, etc.)
    // Sensor pins
    // Configure pins
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    // TODO: name your device to avoid conflictions
    BLEDevice::init("Wayne's Xiao");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setValue("Hello Wayne!!!");
    pService->start();
    // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now you can read it in your phone!");
    
    // Initialize all the readings to 0
    for (int thisReading = 0; thisReading < numReadings; thisReading++) {
        readings[thisReading] = 0;
    }
}

void loop() {
    // TODO: add codes for handling your sensor readings (analogRead, etc.)
    // Read the sensor data
    int rawData = measureDistance();
    // TODO: use your defined DSP algorithm to process the readings
    // Subtract the last reading
    total = total - readings[readIndex];
    // Read from the sensor
    readings[readIndex] = rawData;
    // Add the reading to the total
    total = total + readings[readIndex];
    // Advance to the next position in the array
    readIndex = readIndex + 1;

    // If we're at the end of the array, wrap around to the beginning
    if (readIndex >= numReadings) {
        readIndex = 0;
    }

    // Calculate the average
    average = total / numReadings;
    // Print raw and denoised data
    Serial.print("Raw data: ");
    Serial.print(rawData);
    Serial.print(" Denoised data: ");
    Serial.println(average);

    if (deviceConnected) {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis;
            if (average < 30) {
                pCharacteristic->setValue("Distance: " + String(average) + " cm");
                pCharacteristic->notify();
                Serial.println("Notify value: Distance: " + String(average) + " cm");
            }
        }
    }

    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);  // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising();  // advertise again
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
    delay(1000);
}