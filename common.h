#ifndef COMMON_H
#define COMMON_H

#include <cstdint>

enum MessageType : uint32_t{
    Ready,
    Start,
    Input,
    InputResponse,
    Win,
    Lose,
    RivalState,
    RivalStateInBetween,
    GState,
    GStateField,
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

struct GameState{
    PlayerState player;
    PlayerState rival;
};

struct GameStateField{
    PlayerStateField player;
    PlayerStateField rival;
};

#endif