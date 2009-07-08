#include "std.h"
#include "base.h"

#include <linux/watchdog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <syslog.h>

int std_wd_exists(void) {
	if (access("/dev/watchdog", F_OK) == 0)
		return 1;
	return 0;
}

int std_wd_init(wds *wd, int timeout) {

	wd->type = WDT_STANDART;
	wd->fd = open("/dev/watchdog", O_WRONLY);
	if (wd->fd == -1) {
		syslog(LOG_ERR, "Can not open watchdog device, watchdog initialization fail");
		return -1;
	}

	std_wd_setTimeout(wd,timeout);
	std_wd_start(wd);
	std_wd_ping(wd); // Just for fun
	return 0;
}


void std_wd_start(wds *wd) {
	syslog(LOG_DEBUG, "StdWd: Start");
	int flags = WDIOS_ENABLECARD;
	ioctl(wd->fd, WDIOC_SETOPTIONS, &flags);
}

void std_wd_stop(wds *wd) {
	syslog(LOG_DEBUG, "StdWd: Stop");
	int flags = WDIOS_DISABLECARD;
	ioctl(wd->fd, WDIOC_SETOPTIONS, &flags);
}

void std_wd_setTimeout(wds *wd, int timeout) {
	syslog(LOG_DEBUG, "StdWd: Set timeout: %d", timeout);
	ioctl(wd->fd, WDIOC_SETTIMEOUT, &timeout);
}

void std_wd_ping(wds *wd) {
	syslog(LOG_DEBUG, "StdWd: Ping");
	int dummy;
	ioctl(wd->fd, WDIOC_KEEPALIVE, &dummy);
}
