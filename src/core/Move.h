#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace rubik {

enum class Face : uint8_t {
    F, // Front  (+Z)
    B, // Back   (-Z)
    U, // Up     (+Y)
    D, // Down   (-Y)
    L, // Left   (-X)
    R  // Right  (+X)
};

enum class Direction : uint8_t {
    CW,  // Clockwise (90°)
    CCW  // Counter-clockwise (90°)
};

struct Move {
    Face face;
    Direction direction;

    Move inverse() const;

    // 'f' = F CW, 'F' = F CCW, etc. (as per interface-specification)
    static std::optional<Move> fromChar(char c);
    char toChar() const;

    bool operator==(const Move& other) const;
    bool operator!=(const Move& other) const;
};

// Parse a vector of moves from JSON-style string list: ["f","F","b", ...]
std::vector<Move> parseMovesFromStrings(const std::vector<std::string>& strings);

// Convert moves to string list for serialization
std::vector<std::string> movesToStrings(const std::vector<Move>& moves);

} // namespace rubik
