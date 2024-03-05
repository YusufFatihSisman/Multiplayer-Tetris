#include <cstdint>

enum MessageType : uint32_t{
    Ready,
    Start,
    Input,
    NewPiece,
    Win,
    Lose,
    Damage,
};

struct InputBody{
    bool inputs[4];
    uint32_t counter;
    uint32_t clearCounter;
    uint32_t speed;
};