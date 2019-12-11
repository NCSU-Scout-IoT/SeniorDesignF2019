// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include <WiFi.h>
#include "AzureIotHub.h"
#include "Esp32MQTTClient.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define DEVICE_ID "Esp32Device"
#define MESSAGE_MAX_LEN 256

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// Non volatile variables
RTC_DATA_ATTR int baud_rate = 115200;

RTC_DATA_ATTR char ssid[256] = "\0";
RTC_DATA_ATTR char password[256] = "\0";
//

// BLE stuff
BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
RTC_DATA_ATTR bool wifiMode = false;
//

String inputString = "";
bool commandReady = false;

/*String containing Hostname, Device Id & Device Key in the format:                         */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"                */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessSignature=<device_sas_token>"    */
RTC_DATA_ATTR char connectionString[256] = "\0";

const char *messageData = "{\"deviceId\":\"%s\", \"messageId\":%d, \"%s\":%s}";

int messageCount = 1;
RTC_DATA_ATTR bool hasSSID = false;
RTC_DATA_ATTR bool hasPass = false;
RTC_DATA_ATTR bool hasConnectionString = false;
static bool hasWifi = false;
static bool messageSending = true;
static uint64_t send_interval_ms;


//////////////////////////////////////////////////////////////////////////////////////////////////////////
// BLE Stuff
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);

        Serial.println();
        Serial.println("*********");
      }
    }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Utilities
static void InitWifi()
{
  Serial.println("Connecting...");
  WiFi.begin((const char*)ssid, (const char*)password);
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++count > 20) {
      Serial.println("ERR: Could not connect");
      hasWifi = false;
      return;
    }
  }
  hasWifi = true;
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result)
{
  if (result == IOTHUB_CLIENT_CONFIRMATION_OK)
  {
    Serial.println("Send Confirmation Callback finished.");
  }
}

static void MessageCallback(const char* payLoad, int size)
{
  Serial.println("Message callback:");
  Serial.println(payLoad);
}

static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int size)
{
  char *temp = (char *)malloc(size + 1);
  if (temp == NULL)
  {
    return;
  }
  memcpy(temp, payLoad, size);
  temp[size] = '\0';
  // Display Twin message.
  Serial.println(temp);
  free(temp);
}

static int  DeviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size)
{
  LogInfo("Try to invoke method %s", methodName);
  const char *responseMessage = "\"Successfully invoke device method\"";
  int result = 200;

  if (strcmp(methodName, "start") == 0)
  {
    LogInfo("Start sending temperature and humidity data");
    messageSending = true;
  }
  else if (strcmp(methodName, "stop") == 0)
  {
    LogInfo("Stop sending temperature and humidity data");
    messageSending = false;
  }
  else
  {
    LogInfo("No method %s found", methodName);
    responseMessage = "\"No method found\"";
    result = 404;
  }

  *response_size = strlen(responseMessage) + 1;
  *response = (unsigned char *)strdup(responseMessage);

  return result;
}

void readToEndChar(char terminal, char* string) {
  int i = 0;
  while (1) {
    if (Serial.available() > 0) {
      string[i] = Serial.read();
      if (string[i] == terminal) {
        string[i] = '\0';
        break;
      }
      i++;
    }
  }
}

void initAzure() {
  Esp32MQTTClient_SetOption(OPTION_MINI_SOLUTION_NAME, "GetStarted");
  Esp32MQTTClient_Init((const uint8_t*)connectionString, true);

  Esp32MQTTClient_SetSendConfirmationCallback(SendConfirmationCallback);
  Esp32MQTTClient_SetMessageCallback(MessageCallback);
  Esp32MQTTClient_SetDeviceTwinCallback(DeviceTwinCallback);
  Esp32MQTTClient_SetDeviceMethodCallback(DeviceMethodCallback);
}

void initBLE() {
  // Create the BLE Device
  BLEDevice::init("ESP32 BLE");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );

  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      BLECharacteristic::PROPERTY_WRITE
                                          );

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Arduino sketch
void setup()
{
  Serial.begin(baud_rate);
  Serial.println("ESP32 Device");
  Serial.println("Initializing...");
  initBLE();
  hasWifi = false;
  if (hasSSID && hasPass && wifiMode) {
    InitWifi();
    if(connectionString[0] != '\0'){
      initAzure();
    }
  }
  randomSeed(analogRead(0));
  send_interval_ms = millis();
}

void loop()
{
  while (Serial.available()) {
    //
    char inChar = (char)Serial.read();
    //
    inputString += inChar;
    //
    if (inChar == '\r') {
      commandReady = true;
    }
  }

  if (commandReady) {
    commandReady = false;
    int i;
    int j = 0;
    char command[256];
    for (i = 0; inputString[i] != '\0'; i++) {
      command[i] = inputString[i];
    }
    command[i] = '\0';
    Serial.println(command);
    inputString = "";
    if (!strcmp("AT\r", command)) {
      Serial.println("OK");
    } else if (!strncmp("AT+ssid\r", (const char*)command, 8)) {
      Serial.println(ssid);
      Serial.println("OK");
    } else if (!strncmp("AT+ssid=", (const char*)command, 8)) {
      for (i = 8; command[i] != '\r'; i++) {
        ssid[j++] = command[i];
      }
      ssid[j] = '\0';
      hasSSID = true;
      Serial.println("OK");
      if (hasSSID && hasPass) {
        InitWifi();
      }
    } else if (!strncmp("AT+pass\r", (const char*)command, 8)) {
      Serial.println(password);
      Serial.println("OK");
    } else if (!strncmp("AT+pass=", (const char*)command, 8)) {
      for (i = 8; command[i] != '\r'; i++) {
        password[j++] = command[i];
      }
      password[j] = '\0';
      hasPass = true;
      Serial.println("OK");
      if (hasSSID && hasPass) {
        InitWifi();
      }
    } else if (!strncmp("AT+connString\r", command, 14)) {
      Serial.println(connectionString);
      Serial.println("OK");
    } else if (!strncmp("AT+connString=", command, 14)) {
      for (i = 14; command[i] != '\r'; i++) {
        connectionString[j++] = command[i];
      }
      connectionString[j] = '\0';
      initAzure();
      Serial.println("OK");
    } else if (!strncmp("AT+telemetry=", command, 13)) {
      char telemetry[32];
      for (i = 13; command[i] != ','; i++) {
        telemetry[j++] = command[i];
      }
      telemetry[j] = '\0';
      j = 0;
      char value[32];
      i++;
      while (command[i] != '\r') {
        value[j++] = command[i++];
      }
      value[j] = '\0';
      if (wifiMode) {
        char messagePayload[MESSAGE_MAX_LEN];
        snprintf(messagePayload, MESSAGE_MAX_LEN, messageData, DEVICE_ID, messageCount++, telemetry, value);
        if (hasWifi) {
          if (messageSending) {
            Serial.println(messagePayload);
            EVENT_INSTANCE* message = Esp32MQTTClient_Event_Generate(messagePayload, MESSAGE);
            Esp32MQTTClient_Event_AddProp(message, "temperatureAlert", "true");
            Esp32MQTTClient_SendEventInstance(message);
            send_interval_ms = millis();
            Serial.println("OK");
          }
          else {
            Esp32MQTTClient_Check();
          }
        } else {
          Serial.println("ERR: No wifi");
        }
      }
      else {
        char ble_message[32];
        sprintf(ble_message, "%s=%s", telemetry, value);
        Serial.println(ble_message);
        if (deviceConnected) {
          pTxCharacteristic->setValue(ble_message);
          pTxCharacteristic->notify();
          Serial.println("OK");
        } else {
          Serial.println("ERR: Device not connected");
        }
      }
    } else if (!strncmp("AT+mode\r", command, 8)) {
      if (wifiMode) {
        Serial.println("0: WiFi mode");
      } else {
        Serial.println("1: BLE mode");
      }
      Serial.println("OK");
    } else if (!strncmp("AT+mode=", command, 8)) {
      if (command[8] == '0') {
        wifiMode = true;
        if (hasSSID && hasPass) {
          InitWifi();
          if (hasWifi) {
            initAzure();
          }
        }
      } else if (command[8] == '1') {
        wifiMode = false;
      } else {
        Serial.println("?");
        return;
      }
      Serial.println("OK");
    } else if (!strncmp("AT+baud=", command, 8)) {
      String baud_rate_str = "";
      for (i = 8; command[i] != '\r'; i++) {
        baud_rate_str += command[i];
      }
      baud_rate = baud_rate_str.toInt();
      Serial.printf("Changing baud rate to %d\n", baud_rate);
      delay(50);
      Serial.begin(baud_rate);
      delay(50);
      Serial.printf("Baud rate is now %d\n", baud_rate);
      Serial.println("OK");
    } else if (!strncmp("AT+sleep=", command, 9)) {
      String timer = "";
      for (i = 9; command[i] != '\r'; i++) {
        timer += command[i];
      }
      int timer_int = timer.toInt();
      esp_sleep_enable_timer_wakeup(timer_int * 1000000);
      Serial.printf("ESP32 set to sleep for %d seconds\n", timer_int);
      Serial.println("Going to sleep now");
      Serial.flush();
      esp_deep_sleep_start();
    } else {
      Serial.println("?");
      return;
    }

  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}
