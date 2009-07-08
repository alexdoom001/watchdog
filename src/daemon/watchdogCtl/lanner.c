#include "lanner.h"
#include "lanner_wdt.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <syslog.h>


int lanner_wd_exists(void) {
	if (access("/dev/lanner_wdt", F_OK) == 0)
		return 1;
	return 0;
}

int lanner_wd_init(wds *wd, int timeout) {
	wd->type = WDT_LANNER;
	wd->fd = open("/dev/lanner_wdt", O_WRONLY);
		// or O_RDONLY?
	if (wd->fd == -1) {
		syslog(LOG_ERR, "Can not open watchdog device, watchdog initialization fail");
		return -1;
	}
	lanner_wd_setTimeout(wd,timeout);
	lanner_wd_start(wd);
	lanner_wd_ping(wd); // Just for fun
	return 0;
}

void lanner_wd_start(wds *wd) {
	int flags = START_WDT;
	ioctl(wd->fd, IOCTL_START_STOP_WDT, &flags);

	flags = SET_WDTO_STATE_SYSTEM_RESET;
	ioctl(wd->fd, IOCTL_SET_WDTO_STATE, &flags);
}

void lanner_wd_stop(wds *wd) {
	int flags = STOP_WDT;
	ioctl(wd->fd, IOCTL_START_STOP_WDT, &flags);
}

void lanner_wd_setTimeout(wds *wd, int timeout) {
	ioctl(wd->fd, IOCTL_SET_WDTO_TIMER, &timeout);
}

void lanner_wd_ping(wds *wd) {
	int flags = START_WDT;
	ioctl(wd->fd, IOCTL_START_STOP_WDT, &flags);
	// Yes, for Lanner watchdog, `Ping` is the same as `Start`
}
