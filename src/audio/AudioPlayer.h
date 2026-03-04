#pragma once

#include <string>

namespace rubik {

// Cross-platform playback of the "move" sound effect.
// Desktop: SDL_mixer. Web: calls into JS (Module.playMoveSound).
class AudioPlayer {
public:
    AudioPlayer() = default;
    ~AudioPlayer();

    // Non-copyable
    AudioPlayer(const AudioPlayer&) = delete;
    AudioPlayer& operator=(const AudioPlayer&) = delete;

    // Initialize with path to audio file (e.g. "audio.mp3" or "/audio.mp3" on web).
    // On desktop, path is typically relative to executable directory.
    // Returns true if playback will be available.
    bool init(const std::string& path);

    // Play the move sound effect. No-op if init failed or path was empty.
    void playMoveSound();

    // Call when the user performs an input (key/click). On web, this unlocks audio
    // so that the next playMoveSound() is allowed by the browser. No-op on desktop.
    void notifyUserGesture();

    bool isAvailable() const { return available_; }

private:
    bool available_ = false;

#if !defined(__EMSCRIPTEN__)
    void* music_ = nullptr;  // SDL_mixer Mix_Music*
#endif
};

} // namespace rubik
