#pragma once

#include "core/Move.h"

#include <optional>
#include <vector>

namespace rubik {

class History {
public:
    void push(Move move);

    // Undo: returns the inverse move that should be applied, or nullopt.
    std::optional<Move> undo();

    // Redo: returns the move that should be re-applied, or nullopt.
    std::optional<Move> redo();

    bool canUndo() const { return cursor_ > 0; }
    bool canRedo() const { return cursor_ < static_cast<int>(moves_.size()); }

    void clear();

    const std::vector<Move>& moves() const { return moves_; }
    int cursor() const { return cursor_; }

private:
    std::vector<Move> moves_;
    int cursor_ = 0; // points past the last executed move
};

} // namespace rubik
