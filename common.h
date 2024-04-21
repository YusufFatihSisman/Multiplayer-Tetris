#ifndef COMMON_H
#define COMMON_H

#include <cstdint>

enum MessageType : uint32_t{
    Ready,
    Start,
    Input,
    Win,
    Lose,
    RivalStateInBetween,
    GState,
    GStateField,
    GStateField_1_0,
    GStateField_0_1,
};

struct InputBody{
    bool inputs[4];
    int requestOrder;
};

struct PlayerState{
    int nCurrentPiece;
    int nCurrentRotation;
    int nCurrentX;
    int nCurrentY;
    int nScore;
    bool bRotateHold;
    int lastRequest;
};

struct PlayerStateInBetween{
    int nCurrentX;
    int nCurrentY;
    int nCurrentRotation;
};
    
struct PlayerStateField{
    unsigned char field[216];
    PlayerState playerState;
};

#endif