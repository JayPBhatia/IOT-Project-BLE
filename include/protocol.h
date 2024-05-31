#include <stdint.h>
typedef enum {
    UPDATE_COORDS = 0,
    PLAYER_NAME = 1,
    RECORD = 2,
} Command;

typedef struct {
    Command command;
    char pname[8];

    // record: stored in params[1]
    int32_t params[2];

} BLECommandPacket;
