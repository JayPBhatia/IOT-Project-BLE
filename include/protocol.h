#include <stdint.h>
typedef enum {
    UPDATE_COORDS = 0,
    PLAYER_NAME = 1,
    RECORD = 2,
} Command;

typedef struct {
    Command command;
    // char pname[8];

    // record: stored in params[0]
    // update coords: playerid, px, py
    uint32_t chip_id;
    int32_t params[4];

} BLECommandPacket;
