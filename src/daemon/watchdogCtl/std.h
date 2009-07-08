/* watchdogCtl base class*/
#ifndef WATCHDOGCTL_STD_H_
#define WATCHDOGCTL_STD_H_ 1

#include "base.h"

int std_wd_exists(void);
int std_wd_init(wds *wd, int timeout);
void std_wd_start(wds *wd);
void std_wd_stop(wds *wd);
void std_wd_setTimeout(wds *wd, int timeout);
void std_wd_ping(wds *wd);

#endif //WATCHDOGCTL_STD_H_
