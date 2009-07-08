#include <syslog.h>

#include "base.h"
#include "lanner.h"
#include "std.h"

int wd_init(wds *wd, int timeout) {
	syslog(LOG_DEBUG, "Initialising watchdog device...");
	if (lanner_wd_exists()) {
		syslog(LOG_DEBUG, "Lanner watchdog found...");
		return lanner_wd_init(wd, timeout);
	} else if (std_wd_exists()) {
		syslog(LOG_DEBUG, "Standard watchdog found...");
		return std_wd_init(wd, timeout);
	} else {
		syslog(LOG_ERR, "No Watchdog found");
		return -1;
	}
}

void wd_start(wds *wd) {
	syslog(LOG_DEBUG, "Starting watchdog device...");
	if (wd->type == WDT_LANNER) {
		lanner_wd_start(wd);
	} else if (wd->type == WDT_STANDART) {
		std_wd_start(wd);
	}
}

void wd_stop(wds *wd) {
	syslog(LOG_DEBUG, "Stopping watchdog device...");
	if (wd->type == WDT_LANNER) {
		lanner_wd_stop(wd);
	} else if (wd->type == WDT_STANDART) {
		std_wd_stop(wd);
	}
}

void wd_setTimeout(wds *wd, int timeout) {
	syslog(LOG_DEBUG, "Setting watchdog timeout to %d", timeout);
	if (wd->type == WDT_LANNER) {
		lanner_wd_setTimeout(wd,timeout);
	} else if (wd->type == WDT_STANDART) {
		std_wd_setTimeout(wd,timeout);
	}
}

void wd_ping(wds *wd) {
	syslog(LOG_DEBUG, "Ping watchdog device");
	if (wd->type == WDT_LANNER) {
		lanner_wd_ping(wd);
	} else if (wd->type == WDT_STANDART) {
		std_wd_ping(wd);
	}
}
