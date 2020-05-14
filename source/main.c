#include <switch.h>

#include "config.h"
#include "menu.h"

int main(int argc, char **argv) {
    consoleInit(NULL);

    if (checkRequirement() == 0) {
        printf(CONSOLE_ESC(35;1m)
            "IPSwitchへようこそ。 悪をしないでください。\n\n" CONSOLE_ESC(m));

        mainMenu();
    }

    consoleExit(NULL);
    return 0;
}
