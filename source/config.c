#include "config.h"
#include "patch.h"

int checkRequirement() {
      if(!isDirectory(ATMOS_DIR)) {
    	userConfirm("あなたは雰囲気が必要です");
    	return -1;
    }
    return 0;
}
