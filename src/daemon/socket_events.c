#include <stdlib.h>     /* General Utilities */

#include <string.h>
#include <syslog.h>

//#include <unistd.h>     /* Symbolic Constants */
//#include <sys/types.h>  /* Primitive System Data Types */ 
#include <errno.h>      /* Errors */

// Files
#include <sys/stat.h> /* Some file routines, chmod, for example */

//GLib =(
#include <glib.h>
#include <gio/gio.h>


// NetWork
#include <sys/socket.h>	/* Sockets */
#include <sys/un.h>		/* Local Sockets */

// My Headers
#include "../config.h"
#include "wdtd.h"
#include "socket_events.h"
#include "watchdogCtl/base.h"

struct socketData {
	guint id;
	wdtd *WDT;
	GSocket *sock;
};
static struct socketData *data;

gboolean socketAcceptEvent(GIOChannel *source, GIOCondition condition, gpointer arg);
gboolean socketReadEvent  (GIOChannel *source, GIOCondition condition, gpointer arg);

gboolean socketAcceptEvent(GIOChannel *source, GIOCondition condition, gpointer arg) {
	GError *err = NULL;
	GSocket *new_sock = g_socket_accept(data->sock,
			NULL,
			&err);
	if (err == NULL) {

	struct socketData *new_data = malloc(sizeof(struct socketData));

	new_data->WDT  = data->WDT;
	new_data->sock = new_sock;

	int fd = g_socket_get_fd(new_sock);
	GIOChannel *channel = g_io_channel_unix_new(fd);
	new_data->id = g_io_add_watch(
			channel,
			G_IO_IN,
			socketReadEvent,
			new_data);

	g_io_channel_unref(channel);

	}

	return TRUE;

}

gboolean socketReadEvent(GIOChannel *source, GIOCondition condition, gpointer arg) {
	struct socketData *mydata = (struct socketData*) arg;

	char buf[BUFF_SIZE+1];
	gssize buflen = g_socket_receive(
			mydata->sock,
			buf,
			BUFF_SIZE,
			NULL,
			NULL);
	if (buflen <= 0) { //TODO Ho-Ho!
		g_object_unref(mydata->sock);
		free(mydata);
		return FALSE;
	} else { //process buff
		buf[buflen] = '\0';
		syslog(LOG_DEBUG, "New message recived in local socket: '%s'", buf);
		char res[BUFF_SIZE+1] = "error";
		char *args;

		g_strstrip(buf);
		args = strchr(buf, ' ');
		if (args == NULL) {
			args = &buf[strlen(buf)]; // zero-length string;
		} else {
			*args = '\0';
			args++;
		}
		syslog(LOG_DEBUG, "Command: '%s', Arguments: '%s'", buf, args);

		if (strcmp(buf, "ping") == 0) {
			wd_ping(&mydata->WDT->wdt_ctl);
			//buf="ok";
			strcpy(res,"ok");
		} else if (strcmp(buf, "stage") == 0) {
			//arg should be digit
			//TODO: Should i ckeck for errors?
			int stageN = strtol(args, (char **)NULL, 10);
			mydata->WDT->status = stageN; // Any status is fine, even wrong status
			strcpy(res,"ok");
		} else if (strcmp(buf, "timeout") == 0) {
			int timeout = strtol(args, (char **)NULL, 10);
			if (timeout < 5 || timeout > 255) {
				syslog(LOG_WARNING, "Wrong timeout recieved: '%d', setting to 255", timeout);
				timeout = 255;
			}
			wd_setTimeout(&mydata->WDT->wdt_ctl, timeout);
			mydata->WDT->timeout = timeout;
			strcpy(res,"ok");
			//wd_ping(&data->WDT->wdt_ctl);
		} else if (strcmp(buf, "start") == 0) {
			wd_start(&mydata->WDT->wdt_ctl);
			strcpy(res,"ok");
		} else if (strcmp(buf, "stop") == 0) {
			wd_stop(&mydata->WDT->wdt_ctl);
			strcpy(res,"ok");
		} else if (strcmp(buf, "reload") == 0) {
			char *cf;
			wdtd *wdt;

			if (args[0] == '\0')
				cf = config_file;
			else
				cf = args;

			wdt = malloc(sizeof(wdtd));
			memset(wdt, 0, sizeof(wdtd));
			if (readSettings(wdt, cf) == 0) {
				wdt->timeout = wdt->config.timeout;
				config_wdt(wdt);
				// Global data wdt
				data->WDT = wdt;
				strcpy(res,"ok");
			}
		}

		g_socket_send(
				mydata->sock,
				res,
				strlen(res),
				NULL,
				NULL);
	}

	return TRUE;
}


int addSocketEvents(wdtd *WDT) {
	//First - create socket
	char *socket_path = WDT->config.socket_path;
	GSocket *srvSocket = g_socket_new(
			G_SOCKET_FAMILY_UNIX,
			G_SOCKET_TYPE_STREAM,
			0,
			NULL //Error ref
			);

	// Bind socket to addr
	if (WDT->config.unlink_socket == 1) {
		unlink(socket_path);
	}

	struct sockaddr_un localSockAddr;
	localSockAddr.sun_family = AF_LOCAL;
	strcpy(localSockAddr.sun_path, socket_path);
	/* The size of the address is
	   the offset of the start of the filename,
	   plus its length,
	   plus one for the terminating null byte.
	   Alternatively you can just do:
	   size = SUN_LEN (&name);
	 */

	GSocketAddress *gSockAddr = g_socket_address_new_from_native (&localSockAddr, SUN_LEN(&localSockAddr));

	if (!g_socket_bind(srvSocket, gSockAddr, FALSE, NULL)) {
		syslog(LOG_ERR, "Error in bind(%s)", localSockAddr.sun_path);
		return -1;
	}

	//chmod our socket
	int socket_mod = WDT->config.socket_mode;
	if (socket_mod > 0) {
		chmod(localSockAddr.sun_path, socket_mod);
	}

	data = malloc(sizeof(struct socketData));
	data->WDT  = WDT;
	data->sock = srvSocket;

	int fd = g_socket_get_fd(srvSocket);
	GIOChannel *channel = g_io_channel_unix_new(fd);
	data->id = g_io_add_watch(
			channel, 
			G_IO_IN,
			socketAcceptEvent,
			data);

	g_io_channel_unref(channel);
	// Listen Server socket
	g_socket_listen(srvSocket, NULL);
	//unlink(socket_path); //So... somewhere else

	return 0;
}
