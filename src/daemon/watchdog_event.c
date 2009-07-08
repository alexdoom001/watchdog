#include <stdlib.h>     /* General Utilities */
#include <string.h>

#include <sys/wait.h>	/* for waitpid() */

#include <time.h> 	/* for time(); */

#include <syslog.h>
#include <errno.h>

#include <glib.h>

// My Headers
#include "watchdog_event.h"
#include "watchdogCtl/base.h"
#include "wdtd.h"


struct wdtData {
	guint id;
	int timeout;
	time_t startTime;
	wdtd *WDT;
	int wasFail;
};

static struct wdtData *data;

gboolean watchdog_cb(gpointer arg);


gboolean watchdog_cb(gpointer arg) {
	wdtd *WDT = data->WDT; // Just alias

	// Stale data after config reload, kill timer
	if (arg != data)
		return FALSE;

	switch(WDT->status) {
		case WDTS_INIT: //Wait for stage_0_timeout seconds and fail :)
			{
				time_t now = time(NULL);
				int max_delay = WDT->config.stage_0_timeout;
				if ( (now - data->startTime) <= max_delay ) {
					wd_ping(&WDT->wdt_ctl);
				}
				break;
			}
		case WDTS_INIT_2: //Just ping watchdog device and wait for next stage!
			{
				wd_ping(&WDT->wdt_ctl);
				break;
			}
		case WDTS_STANDART:
			{
				//Fork & Exec checking script
				/*Spawn a child to run the program.*/
				char *check_script = WDT->config.check_script;
				pid_t pid;
				int res = 0;

				if (check_script != NULL) {
					pid = fork();
					if (pid==0) { /* child process */
						syslog(LOG_DEBUG, "Try to execute '%s'", check_script);
						execv(check_script, WDT->config.check_script_params);
						syslog(LOG_ERR, "Can not execute checker script, Error#%d: %s", errno, strerror(errno));
						exit(-1); /* only if execl fails */
					} else if ( pid < 0) { //Some error
						syslog(LOG_ERR, "Can not fork");
						res = -1;
					} else { /* pid!=0; parent process */
						waitpid(pid,&res,0); /* wait for child to exit */
					}
				}
				if (res == 0) {
					syslog(LOG_DEBUG, "System checking: Succes");
					data->wasFail = 0;
					wd_ping(&WDT->wdt_ctl);
				} else {
					data->wasFail = 1;
					syslog(LOG_DEBUG, "System checking: Failed (%d)", res);
				}
				break;
			}
		case WDTS_FAIL: //FAIL, allow watchdog device to kill everyone!
		default:
			//Do nothing, He-he
			; //main.cpp:145:3: error: expected primary-expression before ‘}’ token =(
	}

	int delay;
	if (data->wasFail)
		delay = (WDT->timeout * 1000) / 15;
	else
		delay = (WDT->timeout * 1000) / 3;

	if (delay < 1000) { delay = 1000; }
	if (delay != data->timeout) {
		//If timeout has changed, reinit timeout event
		data->timeout = delay;
		data->id = g_timeout_add_full(
				G_PRIORITY_HIGH, //Hight priority: Watchdog timer is very important service
				data->timeout, // Timeout, in ms
				watchdog_cb, //CB Function
				data,		//Data for CB Function
				NULL // GDestroyNotify notify
				);

		return FALSE;
	} else {
		return TRUE;
	}
}

void addWatchdogEvent(wdtd *WDT) {

	struct wdtData *newdata, *olddata;

	newdata = malloc(sizeof(struct wdtData));
	memset(newdata, 0, sizeof(struct wdtData));

	olddata = data;
	newdata->WDT = WDT;

	if (olddata != NULL && olddata->WDT != NULL) {
		newdata->WDT->status = olddata->WDT->status;
		newdata->WDT->wdt_ctl = olddata->WDT->wdt_ctl;
	}
	if (WDT->config.skip_init_stage == 1 && WDT->status == WDTS_INIT)
		WDT->status = WDTS_STANDART;

	newdata->timeout = (WDT->timeout * 1000) / 3;
	newdata->wasFail = 0;
	if (newdata->timeout < 1000)
		newdata->timeout = 1000;
	newdata->id = g_timeout_add_full(
		G_PRIORITY_HIGH, //Hight priority: Watchdog timer is very important service
		newdata->timeout, // Timeout, in ms
		watchdog_cb, //CB Function
		newdata, //Data for CB Function
		NULL // GDestroyNotify notify
		);

	wd_ping(&WDT->wdt_ctl);
	if (olddata != NULL && olddata->startTime != 0)
		newdata->startTime = olddata->startTime;
	else
		newdata->startTime = time(NULL);
	data = newdata;
	if (olddata != NULL) {
		if (olddata->WDT != NULL) {
			if (olddata->WDT->config.check_script_params != NULL) {
				int i;
				for (i = 0; olddata->WDT->config.check_script_params[i] != NULL; i++) {
					free(olddata->WDT->config.check_script_params[i]);
					// Avoid double free()
					if (olddata->WDT->config.check_script == 
					    olddata->WDT->config.check_script_params[i])
						olddata->WDT->config.check_script = NULL;
				}
			}
			free(olddata->WDT->config.check_script_params);
			free(olddata->WDT->config.check_script);
			free(olddata->WDT->config.socket_path);
			free(olddata->WDT->config.chroot_dir);
			free(olddata->WDT);
		}
		free(olddata);
	}
}
