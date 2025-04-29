/*******************************************************************************
 * ESP32 Bluetooth Presentation Controller
 * ====================================
 *
 * A BLE-enabled presentation remote control system that interfaces with a Python
 * presentation viewer. This device acts as a BLE server that sends presentation
 * control commands to a connected client.
 *
 * Features:
 * - BLE Server functionality with notification support
 * - Dual button control interface (UP/DOWN)
 * - Long press detection for SELECT command
 * - Debounced button input handling
 * - Auto-reconnection capability
 * - Status monitoring via Serial output
 *
 * Hardware Requirements:
 * - ESP32 Development Board
 * - 2x Push Buttons (connected to GPIO 12 and 13)
 * - Pull-down resistors for buttons
 *
 * Technical Specifications:
 * - BLE Protocol: GATT Server
 * - MTU Size: 517 bytes
 * - Commands: UP, DOWN, SELECT
 * - Button Debounce: 1500ms
 * - Long Press Time: 1000ms
 *
 ******************************************************************************/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;

// add your UUIDs here and make sure it matches with presentation_controller.py
#define SERVICE_UUID        "" 
#define CHARACTERISTIC_UUID ""

// GPIO Definitions
#define UP_BUTTON 12
#define DOWN_BUTTON 13

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Client Connected!");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Client Disconnected!");
      BLEDevice::startAdvertising();
      Serial.println("Advertising restarted");
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Server...");

  // Setup GPIO
  pinMode(UP_BUTTON, INPUT_PULLDOWN);
  pinMode(DOWN_BUTTON, INPUT_PULLDOWN);

  // Create the BLE Device
  BLEDevice::init("ESP32_TEST");
  BLEDevice::setMTU(517);
  Serial.println("BLE Device initialized");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  Serial.println("BLE Server created");

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  Serial.println("BLE Service created");

  // Create BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                     CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_WRITE |
                     BLECharacteristic::PROPERTY_NOTIFY
                   );
                   
  pCharacteristic->addDescriptor(new BLE2902());
  
  BLE2902* p2902 = (BLE2902*)pCharacteristic->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  if (p2902) {
    p2902->setNotifications(true);
  }
  
  pCharacteristic->setValue("Ready");
  Serial.println("BLE Characteristic created");

  pService->start();
  Serial.println("BLE Service started");

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->setMinInterval(0x20);   
  pAdvertising->setMaxInterval(0x40);   
  BLEDevice::startAdvertising();
  Serial.println("BLE Advertising started");
  
  Serial.print("Device MAC Address: ");
  Serial.println(BLEDevice::getAddress().toString().c_str());
}

void loop() {
  if (deviceConnected) {
    static bool upButtonPressed = false;
    static bool downButtonPressed = false;
    static unsigned long upPressStartTime = 0;
    static unsigned long downPressStartTime = 0;
    static unsigned long lastCommandSent = 0;
    static const unsigned long LONG_PRESS_TIME = 1000; // 1 second for long press
    static const unsigned long COMMAND_DEBOUNCE = 1500; // 1.5 second between commands
    static bool selectCommandSent = false;
    
    unsigned long currentTime = millis();
    
    // Check UP button
    if (digitalRead(UP_BUTTON) == HIGH) {
      if (!upButtonPressed) {
        upButtonPressed = true;
        upPressStartTime = currentTime;
        selectCommandSent = false;
      } else if (!selectCommandSent && (currentTime - upPressStartTime >= LONG_PRESS_TIME)) {
        // Only send SELECT once per long press
        if (currentTime - lastCommandSent >= COMMAND_DEBOUNCE) {
          pCharacteristic->setValue("SELECT");
          pCharacteristic->notify();
          Serial.println("UP long press - SELECT");
          lastCommandSent = currentTime;
          selectCommandSent = true;
        }
      }
    } else {
      if (upButtonPressed) {
        // Short press handling - only if we didn't already send a SELECT
        if (!selectCommandSent && (currentTime - upPressStartTime < LONG_PRESS_TIME) && 
            (currentTime - lastCommandSent >= COMMAND_DEBOUNCE)) {
          pCharacteristic->setValue("UP");
          pCharacteristic->notify();
          Serial.println("UP button pressed");
          lastCommandSent = currentTime;
        }
        upButtonPressed = false;
      }
    }
    
    // Check DOWN button - same logic as above
    if (digitalRead(DOWN_BUTTON) == HIGH) {
      if (!downButtonPressed) {
        downButtonPressed = true;
        downPressStartTime = currentTime;
        selectCommandSent = false;
      } else if (!selectCommandSent && (currentTime - downPressStartTime >= LONG_PRESS_TIME)) {
        if (currentTime - lastCommandSent >= COMMAND_DEBOUNCE) {
          pCharacteristic->setValue("SELECT");
          pCharacteristic->notify();
          Serial.println("DOWN long press - SELECT");
          lastCommandSent = currentTime;
          selectCommandSent = true;
        }
      }
    } else {
      if (downButtonPressed) {
        if (!selectCommandSent && (currentTime - downPressStartTime < LONG_PRESS_TIME) && 
            (currentTime - lastCommandSent >= COMMAND_DEBOUNCE)) {
          pCharacteristic->setValue("DOWN");
          pCharacteristic->notify();
          Serial.println("DOWN button pressed");
          lastCommandSent = currentTime;
        }
        downButtonPressed = false;
      }
    }
  }
  
  // Print status every 2 seconds
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 2000) {
    Serial.print("Status: ");
    Serial.println(deviceConnected ? "Connected" : "Waiting for connection...");
    lastPrint = millis();
  }
  
  delay(10);
}
