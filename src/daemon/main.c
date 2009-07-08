/#define __GNU_SOURCE 1		/* for getline() -_-*/

#include <stdlib.h>     /* General Utilities */
#include <stdio.h>

#include <string.h>
#include <syslog.h>
#include <errno.h>

//GLib =(
#include <glib.h>
//#include <event.h>


// My Headers
#include "../config.h"
#include "wdtd.h"
#include "watchdogCtl/base.h"
#include "watchdog_event.h"
#include "socket_events.h"

void usage(void);

char *config_file;
int log_level;
// Show usage information
void usage(void) {
	printf("Altell NEO Watchdog daemon\n"
		"Version: " VERSION "\n"
		"\n"
		"-d		Increase debug level\n"
		"-b		Go to background [default]\n"
		"-f		Do not go to background" 
		".\n"
		"-c <file path>	Set path to config file [default: " CONFIG_FILE "].\n"
		"-h		Show this help.\n"
		);
}

int readSettings(wdtd *WDT, char *config) {
	unsigned int i;
	GError *Error = NULL;
	GKeyFile* confParser;
	gboolean gres;

	confParser = g_key_file_new();
	gres = g_key_file_load_from_file(confParser, config, G_KEY_FILE_NONE, &Error);
	syslog(LOG_DEBUG, "Parsing config file '%s'", config);
	if (gres != TRUE ) {
		syslog(LOG_ERR, "Can not parse config file '%s': %s", config,Error->message);
		g_key_file_free(confParser);
		return -1;
	}

	WDT->config.baseloglevel = g_key_file_get_integer(confParser, GSECTION_NAME, "log_level", &Error);
	if (Error != NULL) {
		WDT->config.baseloglevel = -1;
		g_error_free (Error);
	}

	WDT->config.daemonize = g_key_file_get_integer(confParser, GSECTION_NAME, "daemonize", &Error);
	if (Error != NULL) {
		WDT->config.daemonize = -1;
		g_error_free (Error);
	}

	WDT->config.chroot_dir = g_key_file_get_string(confParser, GSECTION_NAME, "chroot", NULL);
	if (WDT->config.chroot_dir != NULL && WDT->config.chroot_dir[0] == '\0') {
		free(WDT->config.chroot_dir);
		WDT->config.chroot_dir = NULL;
	}

	WDT->config.check_script = g_key_file_get_string(confParser, GSECTION_NAME, "check_script", NULL);
	if (WDT->config.check_script != NULL && WDT->config.check_script[0] != '\0') {
		gsize check_services_len = 0;
		char **check_services = NULL;

		if (access(WDT->config.check_script, X_OK) != 0) {
			syslog(LOG_ERR, "File '%s' does not exist or not executable",
			       WDT->config.check_script);
			free(WDT->config.check_script);
			WDT->config.check_script = NULL;
			g_key_file_free(confParser);
			return -1;
		}

		//Check optional check_script parameters, prepare argv
		check_services = g_key_file_get_string_list(confParser, GSECTION_NAME, "check_services",
							    &check_services_len, NULL);
		WDT->config.check_script_params = malloc(sizeof(char *) * (2 + check_services_len));
		if (WDT->config.check_script_params == NULL) {
			syslog(LOG_ERR, "no memory for check script params");
			g_key_file_free(confParser);
			return -1;
		}
		WDT->config.check_script_params[0] = WDT->config.check_script;
		for (i = 0; i < check_services_len; i++)
			WDT->config.check_script_params[i + 1] = check_services[i];

		WDT->config.check_script_params[i + 1] = NULL;
	} else {
		syslog(LOG_NOTICE, "no service checks configured, just kicking the dog");
		free(WDT->config.check_script);
		WDT->config.check_script = NULL;
	}

	//Local socket path
	WDT->config.socket_path = g_key_file_get_string(confParser, GSECTION_NAME, "socket", NULL);
	if (WDT->config.socket_path == NULL || WDT->config.socket_path[0] == '\0') {
		syslog(LOG_ERR, "Local socket path is empty, aborting");
		g_key_file_free(confParser);
		return -1;
	}

	//Socket mode
	char *socket_mode = g_key_file_get_string(confParser, GSECTION_NAME, "socket_mode", NULL);
	if (socket_mode == NULL || socket_mode[0] == '\0') {
		syslog(LOG_ERR, "Can not get local socket mode, aborting");
		g_key_file_free(confParser);
		return -1;
	}
	errno=0;
	WDT->config.socket_mode = strtol(socket_mode, (char **)NULL, 0);
	if (errno != 0) {
		WDT->config.socket_mode = 0;
		syslog(LOG_ERR, "Conversion error for local socket mode, aborting");
		g_key_file_free(confParser);
		return -1;
	}  

	//Timeout
	WDT->config.timeout = g_key_file_get_integer(confParser, GSECTION_NAME, "timeout", &Error);
	if (Error != NULL) {
		g_error_free (Error);
		syslog(LOG_ERR, "Can not get watchdog timeout, aborting");
		g_key_file_free(confParser);
		return -1;
	}
	if (WDT->config.timeout < 5 || WDT->config.timeout > 255) {
		syslog(LOG_WARNING, "Wrong timeout in config file: '%d'", WDT->config.timeout);
		g_key_file_free(confParser);
		return -1;
	}

	WDT->config.nice = g_key_file_get_integer(confParser, GSECTION_NAME, "nice", &Error);
	if (Error != NULL) {
		WDT->config.nice = 0;
		g_error_free (Error);
	}

	WDT->config.unlink_socket = g_key_file_get_integer(confParser, GSECTION_NAME, "unlink_socket", &Error);
	if (Error != NULL) {
		WDT->config.unlink_socket = 0;
		g_error_free (Error);
	}

	WDT->config.stage_0_timeout = g_key_file_get_integer(confParser, GSECTION_NAME, "stage_0_timeout", &Error);
	if (Error != NULL) {
		g_error_free (Error);
		syslog(LOG_ERR, "Can not get 'stage_0_timeout', aborting");
		g_key_file_free(confParser);
		return -1;
	}

	WDT->config.skip_init_stage = g_key_file_get_integer(confParser, GSECTION_NAME, "skip_init_stage", &Error);
	if (Error != NULL) {
		g_error_free (Error);
		syslog(LOG_ERR, "Can not get 'skip_init_stage', aborting");
		g_key_file_free(confParser);
		return -1;
	}
	

	syslog(LOG_DEBUG, "Settings successfully read");
	g_key_file_free(confParser);
	return 0;
}

int config_wdt(wdtd *WDT)
{
	if (WDT->config.baseloglevel > log_level)
		setlogmask(LOG_UPTO(WDT->config.baseloglevel));

	if (WDT->config.nice != 0)
		nice(WDT->config.nice);
	addWatchdogEvent(WDT);
	wd_setTimeout(&WDT->wdt_ctl, WDT->timeout);

	return 0;
}

int main(int argc, char** argv) {
	int daemonize = -1;
	char opt;
	wdtd *WDT;
	GMainLoop* loop;

	log_level = LOG_ERR;
	config_file = CONFIG_FILE;
	//Open syslog
	openlog("neo-watchdogd", LOG_CONS, DEFAULT_FACILITY); //Open syslog

	//Initialize GLib -_-
	g_type_init();

	// Read command line options
	while ((opt = getopt(argc,argv,"c:bfdh")) != -1) {
		switch (opt) {
		case 'c':
			config_file = optarg;
			syslog(LOG_DEBUG, "Set config file to '%s'", optarg);
			break;
		case 'b':
			daemonize = 1;
			syslog(LOG_DEBUG, "Set daemonization flag to 1");
			break;
		case 'f':
			daemonize = 0;
			syslog(LOG_DEBUG, "Set daemonization flag to 0");
			break;
		case 'd':
			log_level = LOG_DEBUG;
			setlogmask(LOG_UPTO(log_level));
			syslog(LOG_DEBUG, "Increasing log level");
			break;
		case 'h':
			usage();
			exit(0);
		}
	}

	WDT = malloc(sizeof(wdtd));
	if (WDT == NULL) {
		syslog(LOG_ERR, "OOM");
		return 1;
	}

	memset(WDT, 0, sizeof(wdtd));
	WDT->status = WDTS_INIT;

	if (readSettings(WDT, config_file) == -1) {
		syslog(LOG_ERR, "Error while reading settings");
		free(WDT);
		return 2;
	}

	// daemonize, chroot and socket settings can only be set on start
	if (daemonize >= 0)
		WDT->config.daemonize = daemonize;
	if (WDT->config.daemonize < 0)
		WDT->config.daemonize = 1;

	if (WDT->config.daemonize == 1) {
		syslog(LOG_DEBUG, "Daemonizing...");
		if (daemon(0, 0) != 0) {
			syslog(LOG_ERR, "Daemonization error: #%d: %s", errno, strerror(errno));
			return 3;
		}
	}

	if (WDT->config.chroot_dir != NULL && chroot(WDT->config.chroot_dir) != 0) {
		syslog(LOG_ERR, "Error chrooting: (%d) %s", errno, strerror(errno));
		return 4;
	}

	loop = g_main_loop_new(NULL, FALSE);

	WDT->timeout = WDT->config.timeout;
	if (wd_init(&WDT->wdt_ctl, WDT->timeout) == -1) {
		syslog(LOG_ERR, "No Lanner, nor standard watchdog found");
		return 5;
	}

	if (config_wdt(WDT) != 0) {
		syslog(LOG_ERR, "Error configuring watchdog");
		return 6;
	}

	if (addSocketEvents(WDT) == 0)
		g_main_loop_run(loop);
	else {
		syslog(LOG_ERR, "Failed to add watchdog and socket events");
		return 10;
	}

	//Free Memory & etc..
	g_main_loop_unref(loop);

	free(WDT);

	closelog(); //Close syslog

	return 0;
}
