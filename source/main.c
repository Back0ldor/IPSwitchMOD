#include <switch.h>

#include "config.h"
#include "menu.h"

int main(int argc, char **argv) {
    gfxInitDefault();
    consoleInit(NULL);

    if (checkRequirement() == 0) {
        printf(CONSOLE_ESC(35;1m)
            "Welcome to IPSwitch[ReiNX MOD]. Do no evil.\n\n" CONSOLE_ESC(m));

        mainMenu();
    }

    gfxExit();
    return 0;
}
