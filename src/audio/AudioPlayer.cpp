#include "audio/AudioPlayer.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#elif defined(RUBIK_HAVE_SDL2_MIXER)
#include <SDL_mixer.h>
#endif

namespace rubik {

AudioPlayer::~AudioPlayer() {
#if defined(RUBIK_HAVE_SDL2_MIXER) && !defined(__EMSCRIPTEN__)
    if (music_) {
        Mix_FreeMusic(static_cast<Mix_Music*>(music_));
        music_ = nullptr;
    }
#endif
}

bool AudioPlayer::init(const std::string& path) {
    if (path.empty()) return false;

#ifdef __EMSCRIPTEN__
    (void)path;  // path is fixed in JS as /audio.mp3
    available_ = true;  // assume preloaded; JS will handle missing file
    return true;
#elif defined(RUBIK_HAVE_SDL2_MIXER)
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) < 0)
        return false;
    Mix_Music* m = Mix_LoadMUS(path.c_str());
    if (!m) return false;
    music_ = m;
    available_ = true;
    return true;
#else
    (void)path;
    return false;
#endif
}

void AudioPlayer::playMoveSound() {
    if (!available_) return;

#ifdef __EMSCRIPTEN__
    EM_ASM(
        if (typeof Module !== 'undefined' && typeof Module.playMoveSound === 'function') {
            Module.playMoveSound();
        }
    );
#elif defined(RUBIK_HAVE_SDL2_MIXER)
    if (music_)
        Mix_PlayMusic(static_cast<Mix_Music*>(music_), 0);
#endif
}

void AudioPlayer::notifyUserGesture() {
#ifdef __EMSCRIPTEN__
    EM_ASM(
        if (typeof Module !== 'undefined' && typeof Module.unlockAudioForMoveSound === 'function') {
            Module.unlockAudioForMoveSound();
        }
    );
#endif
}

} // namespace rubik
