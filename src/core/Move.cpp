#include "core/Move.h"

#include <cctype>

namespace rubik {

Move Move::inverse() const {
    return {face, direction == Direction::CW ? Direction::CCW : Direction::CW};
}

std::optional<Move> Move::fromChar(char c) {
    Face face;
    switch (std::tolower(c)) {
        case 'f': face = Face::F; break;
        case 'b': face = Face::B; break;
        case 'u': face = Face::U; break;
        case 'd': face = Face::D; break;
        case 'l': face = Face::L; break;
        case 'r': face = Face::R; break;
        default: return std::nullopt;
    }
    Direction dir = std::isupper(c) ? Direction::CCW : Direction::CW;
    return Move{face, dir};
}

char Move::toChar() const {
    char base = 0;
    switch (face) {
        case Face::F: base = 'f'; break;
        case Face::B: base = 'b'; break;
        case Face::U: base = 'u'; break;
        case Face::D: base = 'd'; break;
        case Face::L: base = 'l'; break;
        case Face::R: base = 'r'; break;
    }
    return direction == Direction::CCW
               ? static_cast<char>(std::toupper(base))
               : base;
}

bool Move::operator==(const Move& other) const {
    return face == other.face && direction == other.direction;
}

bool Move::operator!=(const Move& other) const {
    return !(*this == other);
}

std::vector<Move> parseMovesFromStrings(const std::vector<std::string>& strings) {
    std::vector<Move> result;
    result.reserve(strings.size());
    for (const auto& s : strings) {
        if (s.size() == 1) {
            if (auto m = Move::fromChar(s[0])) {
                result.push_back(*m);
            }
        }
    }
    return result;
}

std::vector<std::string> movesToStrings(const std::vector<Move>& moves) {
    std::vector<std::string> result;
    result.reserve(moves.size());
    for (const auto& m : moves) {
        result.push_back(std::string(1, m.toChar()));
    }
    return result;
}

} // namespace rubik
