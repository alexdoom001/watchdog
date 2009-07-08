#include <stdlib.h>     /* General Utilities */
#include <stdio.h>

#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>

//GLib =(
#include <glib.h>

// NetWork
#include <sys/socket.h>	/* Sockets */
#include <sys/un.h>		/* Local Sockets */

// My Headers
#include "../config.h"
//#include "../lib/sa-string-f.h"

char *readSettings(int argc, char** argv);
void usage(const char *argv0);
int quiet;

// Show usage information
void usage(const char *argv0) {
	printf("Altell NEO Watchdog client\n"
		"Version: " VERSION "\n"
		"Usage: %s [options] \"command\"\n"
		"\n"
		"Avaliable options:\n"
		"-q		Quiet.\n"
		"-c <file path>	Set path to config file [default: " CONFIG_FILE "].\n"
		"-h		Show this help.\n"
		"\n"
		"Avaliable commands: \n"
		"ping		- Ping watchdog device\n"
		"stage <n>	- Set stage to <n>\n"
		"start		- Start watchdog device timer\n"
		"stop		- Stop watchdog device timer\n"
		"timeout <3-255>	- set watchdog timer timeout\n"
		, 
		argv0
		);
}

char *readSettings(int argc, char** argv) {
	char config_def[] = CONFIG_FILE;
	char *config = config_def;
	// Read command line options

	char opt;
	while ((opt = getopt(argc,argv,"c:hq")) != -1) {
		switch (opt) {
		case 'c':
			config = optarg;
			break;
		case 'h':
			usage(argv[0]);
			exit(0);
			break;
		case 'q':
			quiet = 1;
		}
	}

	GError *Error = NULL;
	GKeyFile* confParser = g_key_file_new ();
	gboolean gres =  g_key_file_load_from_file (confParser, config, G_KEY_FILE_NONE, &Error);
	if (gres != TRUE ) {
		fprintf(stderr, "Can not parse config file '%s': %s", config,Error->message);
		syslog(LOG_ERR, "Can not parse config file '%s': %s", config,Error->message);
		g_key_file_free(confParser);
		return NULL;
	}

	
	//Local socket path
	char *socket_path = g_key_file_get_string(confParser, GSECTION_NAME, "socket", NULL);
	if (socket_path == NULL || socket_path[0] == '\0') {
		fprintf(stderr, "Local socket path is empty, abotring\n");
		syslog(LOG_ERR, "Local socket path is empty, abotring\n");
		g_key_file_free(confParser);
		return NULL;
	}

	g_key_file_free(confParser);
	return socket_path;
}

int main(int argc, char** argv) {

	openlog("neo-watchdog-client", LOG_CONS, DEFAULT_FACILITY); //Open syslog
	char *socket_path = readSettings(argc, argv);

	if (socket_path == NULL) {
		fprintf(stderr, "Error while reading settings\n");
		return -1;
	}

	if (optind >= argc) {
		fprintf(stderr, "Error, command not found");
		return -1;
	}

	int socket_fd;
	struct sockaddr_un name;
	socket_fd = socket (PF_LOCAL, SOCK_STREAM, 0); 
	name.sun_family = AF_LOCAL;
	strcpy (name.sun_path, socket_path);
	if (connect(socket_fd, (struct sockaddr *)&name, SUN_LEN (&name)) == -1) {
		int _errno = errno;
		fprintf(stderr,		"Can not connect to local socket '%s', Error#%d: %s", socket_path, _errno, strerror(_errno) );
		syslog(LOG_ALERT,	"Can not connect to local socket '%s', Error#%d: %s", socket_path, _errno, strerror(_errno) );
		close (socket_fd);
		closelog(); //Close syslog
		return -1;
	}
	int i = optind;
	write(socket_fd, argv[i], strlen(argv[i]));

	char buf[BUFF_SIZE+1];
	int res = read(socket_fd,buf, BUFF_SIZE);
	if (res >= 0) {
		buf[res] = '\0';
		if (!quiet)
			printf("Server response: %s\n", buf);
		if (strcmp(buf, "ok") == 0)
			res = 0;
		else
			res = 1;
	} else
		fprintf(stderr, "Error reading result from server");

	close (socket_fd);
	closelog(); //Close syslog
	return res;
}
