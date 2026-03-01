#include "animation/AnimationManager.h"

#include <algorithm>

namespace rubik {

void AnimationManager::enqueueMove(Move move) {
    queue_.push(move);
    if (state_ == State::Idle) {
        startNextMove();
    }
}

void AnimationManager::enqueueSequence(const std::vector<Move>& moves) {
    for (auto& m : moves) queue_.push(m);
    if (state_ == State::Idle && !queue_.empty()) {
        startNextMove();
    }
}

void AnimationManager::update(float dt) {
    if (state_ != State::Moving) return;

    progress_ += dt / moveDuration;

    if (progress_ >= 1.0f) {
        progress_ = 1.0f;

        if (onMoveComplete) onMoveComplete(currentMove_);
        hasCurrentMove_ = false;

        if (!queue_.empty()) {
            startNextMove();
        } else {
            state_ = State::Idle;
            progress_ = 0.0f;
        }
    }
}

std::optional<Move> AnimationManager::currentMove() const {
    if (hasCurrentMove_) return currentMove_;
    return std::nullopt;
}

void AnimationManager::reset() {
    state_ = State::Idle;
    progress_ = 0.0f;
    hasCurrentMove_ = false;
    queue_ = {};
}

void AnimationManager::startNextMove() {
    currentMove_ = queue_.front();
    queue_.pop();
    hasCurrentMove_ = true;
    state_ = State::Moving;
    progress_ = 0.0f;

    if (onMoveBegin) onMoveBegin(currentMove_);
}

} // namespace rubik
