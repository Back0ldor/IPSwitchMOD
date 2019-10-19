#include "config.h"
#include "patch.h"

int checkRequirement() {
    if(!isDirectory(REINX_DIR)) {
    	userConfirm("ERROR,did you miss something?");
    	return -1;
    }
    return 0;
}
