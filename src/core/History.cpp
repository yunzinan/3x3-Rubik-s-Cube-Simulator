#include "core/History.h"

namespace rubik {

void History::push(Move move) {
    // Discard any redo-able moves after the cursor
    moves_.resize(cursor_);
    moves_.push_back(move);
    ++cursor_;
}

std::optional<Move> History::undo() {
    if (!canUndo()) return std::nullopt;
    --cursor_;
    return moves_[cursor_].inverse();
}

std::optional<Move> History::redo() {
    if (!canRedo()) return std::nullopt;
    Move m = moves_[cursor_];
    ++cursor_;
    return m;
}

void History::clear() {
    moves_.clear();
    cursor_ = 0;
}

} // namespace rubik
