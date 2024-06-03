#include "Adafruit_NeoPixel.h"
#include "ScreenRender.h"
#include <Arduino.h>
#include <Wire.h>

ScreenRender screen { 6, 4, 8 };
uint8_t buffer[32];
PlayerPixel player1;

void parse_buffer() {
    // protocol: id, x, y

    uint8_t id = buffer[0];
    uint8_t x = buffer[1];
    uint8_t y = buffer[2];
    Serial.println(id);
    Serial.println(x);
    Serial.println(y);
    screen.SetPlayerPosition(0, y, x);
}

void receive_handler(int num_bytes) {
    // called on twi write
    Serial.println("rec handler");
    for (int i = 0; i < num_bytes; i++) {
        buffer[i] = Wire.read();
    }
    parse_buffer();
}
void test_screen() {
    player1.id = 1;
    player1.r = 255;
    player1.row = 1;
    player1.col = 1;
    player1.g = 0;
    player1.b = 0;
    int idx = screen.AddPlayer(player1);
    screen.SetPlayerPosition(0, 0, 1);
    screen.UpdateScreen();

    // Adafruit_NeoPixel matrix = Adafruit_NeoPixel(32, 6, NEO_KHZ400);
    // matrix.begin();
    // matrix.fill(0xffffff, 0, 32);
    // matrix.show();
}

void setup_player() {
    player1.id = 1;
    player1.r = 255;
    player1.row = 1;
    player1.col = 1;
    player1.g = 0;
    player1.b = 0;
    int idx = screen.AddPlayer(player1);
}

void setup() {
    Serial.begin(115200);
    Wire.begin(2);
    Wire.onReceive(receive_handler);
    setup_player();
    pinMode(6, OUTPUT);
    // test_screen();
}

void loop() {
    screen.UpdateScreen();
    delay(100);
}
