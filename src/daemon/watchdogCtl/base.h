/* watchdogCtl base class*/
#ifndef WATCHDOGCTL_BASE_H_
#define WATCHDOGCTL_BASE_H_ 1

#include <unistd.h>

#define WDT_STANDART	0
#define WDT_LANNER		1

typedef struct _wds {
	int fd;
	int type;
} wds;

int wd_init(wds *wd, int timeout);
void wd_start(wds *wd);
void wd_stop(wds *wd);
void wd_setTimeout(wds *wd, int timeout);
void wd_ping(wds *wd);

#endif //WATCHDOGCTL_BASE_H_
