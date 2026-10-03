#pragma once
#include <string>

// Everything you can buy for the garden besides seeds and land.
// Lily pads and trees are "placeable" (you click a tile); creatures just appear.
enum class ExtraKind { LilyPad, Frog, Turtle, Bird, OakTree, PineTree, CherryTree };

constexpr int kExtraKindCount = 7;

struct ExtraData {
    ExtraKind kind;
    std::string name;
    std::string blurb;
    int price;
    int max;         // how many you can own
    bool placeable;
};

inline const ExtraData& extraData(ExtraKind k) {
    static const ExtraData kData[kExtraKindCount] = {
        {ExtraKind::LilyPad, "Lily pad", "Floats on the pond. Frogs love them.", 30, 8, true},
        {ExtraKind::Frog, "Frog", "Hops between lily pads.", 100, 4, false},
        {ExtraKind::Turtle, "Turtle", "Paddles slowly around the pond.", 180, 2, false},
        {ExtraKind::Bird, "Songbird", "Perches in trees and pecks about.", 80, 4, false},
        {ExtraKind::OakTree, "Oak tree", "A leafy shade tree.", 50, 6, true},
        {ExtraKind::PineTree, "Pine tree", "Tall, dark and evergreen.", 70, 6, true},
        {ExtraKind::CherryTree, "Cherry tree", "Pink blossoms and drifting petals.", 140, 4, true},
    };
    return kData[int(k)];
}

inline bool isTree(ExtraKind k) {
    return k == ExtraKind::OakTree || k == ExtraKind::PineTree || k == ExtraKind::CherryTree;
}
