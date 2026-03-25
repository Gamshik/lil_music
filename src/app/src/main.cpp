#include "soundcloud/app/desktop_application.h"

int main() {
    // Точка входа не содержит логики, а только запускает composition root приложения.
    soundcloud::app::desktop_application application;
    return application.run();
}
