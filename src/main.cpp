#include "app/DesktopApp.h"

int main(int argc, char** argv) {
    rubik::DesktopApp app{{argc, argv}};
    return app.exec();
}
