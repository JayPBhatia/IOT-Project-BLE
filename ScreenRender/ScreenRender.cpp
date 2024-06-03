#include "ScreenRender.h"
#include "Adafruit_NeoPixel.h"

ScreenRender::ScreenRender(size_t pin, size_t height, size_t width)
    : height_(height)
    , width_(width)
    , objects()
    , num_players_(0)
    , brightness_(255)
    , matrix_(Adafruit_NeoPixel(height * width, pin, NEO_GRB + NEO_KHZ800))
    , shift_val_(0) {
    memset(objects, 0, kMaxPlayers * sizeof(PlayerPixel));
    matrix_.begin();
    matrix_.fill(0, 0, height_ * width_);
}

void ScreenRender::UpdateScreen() {
    matrix_.fill(0, 0, height_ * width_);
    for (int i = 0; i < num_players_; i++) {
        // uint32_t colour = matrix_.getPixelColor(objects[i].row * 8 + objects[i].col);
        // uint8_t r = colour >> 16;
        // uint8_t g = (colour >> 8) & 0xff;
        // uint8_t b = colour & 0xff;
        // uint8_t r = 0;
        // uint8_t g = 0;
        // uint8_t b = 0;
        matrix_.setPixelColor(objects[i].row * 8 + objects[i].col, objects[i].r, objects[i].g, objects[i].b);
    }
    matrix_.show();
}

void ScreenRender::SetColourShift(int colorval) { shift_val_ = colorval; }

int ScreenRender::AddPlayer(PlayerPixel player) {
    if (num_players_ >= kMaxPlayers) {
        return -1;
    }
    int idx = num_players_;

    objects[num_players_++] = player;
    return idx;
}

int ScreenRender::SetPlayerPosition(uint8_t id, uint8_t row, uint8_t col) {
    if (id >= kMaxPlayers) {
        return -1;
    }

    if (row < height_) {

        objects[id].row = row;
    } else {
        return -1;
    }
    if (row < width_) {
        objects[id].col = col;
        return id;
    }
    return -1;
}

int ScreenRender::GetPlayerPixelIdx(size_t deviceid) {
    for (size_t i = 0; i < kMaxPlayers; ++i) {
        if (objects[i].id == deviceid) {
            return i;
        }
    }
    return -1;
}
