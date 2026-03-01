#pragma once

#include "core/Move.h"

#include <functional>
#include <optional>
#include <queue>

namespace rubik {

// Finite State Machine for face-turn animations.
//   Idle   → user input / queue has moves → Moving
//   Moving → animation finished + queue empty → Idle
//   Moving → animation finished + queue non-empty → Moving (next move)
class AnimationManager {
public:
    enum class State { Idle, Moving };

    // Duration of one 90° face turn (seconds).
    float moveDuration = 0.35f;

    // ── Callbacks (set by the application) ──────────────────────────────
    // Called once when a new move animation starts.
    std::function<void(Move)> onMoveBegin;
    // Called once when the move animation completes.
    std::function<void(Move)> onMoveComplete;

    // ── Public API ──────────────────────────────────────────────────────

    // Enqueue a single move.  If idle, starts immediately.
    void enqueueMove(Move move);

    // Enqueue a sequence of moves (e.g. from file import).
    void enqueueSequence(const std::vector<Move>& moves);

    // Call every frame with wall-clock deltaTime (seconds).
    void update(float dt);

    State state() const { return state_; }
    bool  isAnimating() const { return state_ == State::Moving; }

    // Normalised progress of the current animation [0, 1].
    float progress() const { return progress_; }

    // The move currently being animated (valid only while Moving).
    std::optional<Move> currentMove() const;

    // Clear everything (stop animation + empty queue).
    void reset();

private:
    State state_ = State::Idle;
    float progress_ = 0.0f;

    Move  currentMove_{};
    bool  hasCurrentMove_ = false;

    std::queue<Move> queue_;

    void startNextMove();
};

} // namespace rubik
