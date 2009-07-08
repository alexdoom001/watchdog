/* watchdogCtl base class*/
#ifndef WATCHDOGCTL_LANNER_H_
#define WATCHDOGCTL_LANNER_H_ 1

#include "base.h"

int lanner_wd_exists(void);
int lanner_wd_init(wds *wd, int timeout);
void lanner_wd_start(wds *wd);
void lanner_wd_stop(wds *wd);
void lanner_wd_setTimeout(wds *wd, int timeout);
void lanner_wd_ping(wds *wd);

#endif //WATCHDOGCTL_LANNER_H_
