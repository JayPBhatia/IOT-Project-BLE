/*
    Based on Neil Kolban example for IDF:
   https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp Ported to Arduino
   ESP32 by Evandro Copercini updates by chegewara
*/
#include "ScreenRender.h"
#include "protocol.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <Wire.h>
// #include <SD.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914c"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// forward decls
class PacketCallbacks;
bool parse_packet(const BLECommandPacket* packet);
bool handle_update_coords(const BLECommandPacket* packet);
bool handle_record(const BLECommandPacket* packet);
bool handle_playername(const BLECommandPacket* packet);
ScreenRender screen(16, 4, 8);
void setup_screen();
bool is_recording = false;
const uint32_t record_timeout = 30000;
uint32_t record_end_time;

// ble server and services
BLEServer* pServer;
BLEService* pService;
BLECharacteristic* pCharacteristic;

// sd card
bool sd_fail = false;
const int sd_chip_sel = 10;
// File datafile;
// stop recording timeout function
xTaskHandle stop_record_handle;
void stop_record(void* params);

// semaphore for the player update
xSemaphoreHandle player_update_sem;

class PacketCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic);
};

// void PacketCallbacks::onConnect(BLECharacteristic* pCharacteristic) {
//     // connection established
//     Serial.println("Connection established");
// }

void PacketCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
    // get the data from the packet that just arrived
    uint8_t packet_bytes[pCharacteristic->getLength()];
    memcpy(packet_bytes, pCharacteristic->getData(), pCharacteristic->getLength());
    BLECommandPacket* packet = (BLECommandPacket*)packet_bytes;
    Serial.println("Write");
    parse_packet(packet);
}

void update_screen_task(void* params) {
    while (1) {
        bool sem_taken = xSemaphoreTake(player_update_sem, 100);
        if (sem_taken) {
            screen.UpdateScreen();
            xSemaphoreGive(player_update_sem);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void stop_record(void* params) {
    while (1) {
        if (record_end_time <= pdTICKS_TO_MS(xTaskGetTickCount())) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(record_end_time - pdTICKS_TO_MS(xTaskGetTickCount())));
    }

    // stop the recording and delete this task
    is_recording = false;
    vTaskDelete(stop_record_handle);
}

bool parse_packet(const BLECommandPacket* packet) {
    // parses a packet, returns if the packet is valid or not
    Serial.println((size_t)packet->command);
    Serial.println(packet->chip_id);
    switch (packet->command) {
    case Command::UPDATE_COORDS:
        return handle_update_coords(packet);
    case Command::RECORD:
        return handle_record(packet);
    case Command::PLAYER_NAME:
        return handle_playername(packet);
    default:
        return false;
    }
    return false;
}

bool handle_update_coords(const BLECommandPacket* packet) {
    // handle updating coordinates
    Serial.println("Update coords");
    uint8_t buffer[8];
    buffer[0] = packet->chip_id & 0xff;
    buffer[1] = packet->params[0] & 0xff;
    buffer[2] = packet->params[1] & 0xff;
    Serial.println(buffer[0]);
    Serial.println(buffer[1]);
    Serial.println(buffer[2]);
    Wire.beginTransmission(2);
    Wire.write((uint8_t*)buffer, sizeof(buffer));
    Wire.endTransmission();
    return true;
}

bool handle_playername(const BLECommandPacket* packet) {
    // handshake for new player
    PlayerPixel player { .id = packet->chip_id };
    Serial.println(player.id);
    int player_idx = screen.AddPlayer(player);
    pCharacteristic->setNotifyProperty(true);
    pCharacteristic->setValue(player_idx);
    pCharacteristic->notify();
    return player_idx < 0 ? false : true;
}

bool handle_record(const BLECommandPacket* packet) {
    // do nothing first
    record_end_time = pdTICKS_TO_MS(xTaskGetTickCount());
    is_recording = true;

    // write to file
    // datafile = SD.open("data.txt");
    xTaskCreate(stop_record, "stop_record", 1024, NULL, 1, &stop_record_handle);
    return true;
}

void setup_ble() {
    BLEDevice::init("IOT-Project-Server");
    pServer = BLEDevice::createServer();
    pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

    pCharacteristic->setCallbacks(new PacketCallbacks());
    pCharacteristic->setValue("Game BLE ESP Server");
    pService->start();
    // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Setup complete. Advertising...");
}

void setup_sd() {
    // setup the sd card
    // if (!SD.begin(sd_chip_sel)) {
    //     Serial.println("SD init fail");
    //     sd_fail = true;
    //     return;
    // }
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { }
    Serial.println("Starting BLE...");
    setup_ble();
    Serial.println("Setting up i2c");
    Wire.begin();
    // xTaskCreate(update_screen_task, "update_screen", 1024, NULL, 1, NULL);
}

void loop() {
    // put your main code here, to run repeatedly:
}
