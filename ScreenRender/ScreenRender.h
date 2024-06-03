#pragma once
#include "Adafruit_NeoPixel.h"
#include <Arduino.h>

static constexpr size_t kMaxPlayers { 2 };

struct PlayerPixel {
    char name[8];
    size_t id;
    size_t row;
    size_t col;
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

class ScreenRender {
public:
    explicit ScreenRender(size_t pin, size_t height, size_t width);
    void UpdateScreen();
    void SetColourShift(int colorval);
    void SetBrightness(int brightness);
    int AddPlayer(PlayerPixel player);
    int SetPlayerPosition(uint8_t id, uint8_t row, uint8_t col);
    int GetPlayerPixelIdx(size_t playerid);
    size_t height_;
    size_t width_;
    PlayerPixel objects[kMaxPlayers];
    int num_players_;
    uint8_t brightness_;
    Adafruit_NeoPixel matrix_;
    uint8_t shift_val_;

private:
    void ProcessColours();
};
