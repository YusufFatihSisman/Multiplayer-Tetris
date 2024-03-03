//#include <iostream>
#include <cstdint>

enum MessageType : uint32_t{
    Ready,
    Start,
    Input,
    NewPiece,
    Win,
    Lose
};

struct InputBody{
    bool inputs[4];
    uint32_t counter;
    uint32_t clearCounter;
    uint32_t nCurrentPiece;
    uint32_t nCurrentRotation;
    uint32_t nCurrentX;
    uint32_t nCurrentY;
};