#include "app/WebApp.h"

int main(int argc, char** argv) {
    // EmscriptenApplication::exec() returns immediately; the main loop runs
    // via requestAnimationFrame. The app must outlive main() or the callback
    // would use a destroyed object (pure virtual call / crash).
    static rubik::WebApp app{{argc, argv}};
    return app.exec();
}
