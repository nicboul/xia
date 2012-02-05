/*
 * See COPYRIGHTS file
 */


#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/if.h>
#include <linux/if_tun.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "journal.h"
#include "tun.h"

extern int tun_destroy(iface_t *iface)
{
	return -1;
}

extern int tun_up(char *devname, char *addr)
{
	journal_ftrace(__func__);
	
	char sys[128];
	int ret;

	snprintf(sys, 128, "%s %s %s",
		"ifconfig",
		devname,
		addr);

	printf("sys:%s\n", sys);

	ret = system(sys);
	return ret;
}


extern int tun_create(int *devname, int *fd)
{
	journal_ftrace(__func__);

	struct ifreq ifr;
	int ret;
	int dev;

	*fd = open("/dev/net/tun", O_RDWR);
	if (*fd < 0) {
		journal_notice("tun_ifreq]> open tun failed :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	ifr.ifr_name[0] = '\0';	/* Get the next available interface */

	ret = ioctl(*fd, TUNSETIFF, (void *)&ifr);
	if (ret < 0) {
		journal_notice("tun_ifreq]> ioctl TUNSETIFF :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	ret = ioctl(*fd, TUNGETIFF, (void *)&ifr);
	if (ret < 0) {
		journal_notice("tun_ifreq]> ioctl TUNGETIFF :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	snprintf(devname, IFNAMSIZ, "%s", ifr.ifr_name);
	printf("tun_ifreq: devname: %s %s\n", ifr.ifr_name, devname);

	return ret;
}  
