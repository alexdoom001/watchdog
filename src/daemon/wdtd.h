#ifndef WDTD_H_
#define WDTD_H_ 1

#include "watchdogCtl/base.h"

#define WDTS_INIT 		0
#define WDTS_INIT_2 	1
#define WDTS_STANDART 	2
#define WDTS_FAIL	 	3

typedef struct _xConfig {
	char *check_script;
	char **check_script_params;

	char *socket_path;
	char *chroot_dir;
	int socket_mode;
	int unlink_socket;

	int timeout;
	int stage_0_timeout;
	int skip_init_stage;
	int daemonize;
	int baseloglevel;
	int nice;
} xConfig;

typedef struct _wdtd_struct {
	xConfig	config;
	wds wdt_ctl;
	int timeout;
	int status;
} wdtd;

int readSettings(wdtd *WDT, char *config);
int config_wdt(wdtd *WDT);
extern char *config_file;


#endif //WDTD_H_
