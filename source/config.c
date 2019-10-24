#include "config.h"
#include "patch.h"

int checkRequirement() {
    if(!isDirectory(REINX_DIR)) {
    	userConfirm("I only works with ReiNX");
    	return -1;
    }
    return 0;
}
