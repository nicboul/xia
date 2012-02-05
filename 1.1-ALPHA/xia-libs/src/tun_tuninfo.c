/*
 * See COPYRIGHTS file.
 */


#include <sys/types.h>

#include <sys/socket.h> // AF_MAX
#include <net/if.h>
#include <net/if_types.h>
#include <net/if_tun.h>

#include <fcntl.h>

#include "tun.h"

extern int tun_destroy(iface_t *tun)
{
	journal_ftrace(__func__);

	char sys[128];
	int ret;

	ret = close(tun->fd);
	if (ret == -1) {
		journal_notice("tun]> failed closing iface fd [%i] :: %i:%s\n", __LINE__, __FILE__);
	}

	snprintf(sys, 128, "ifconfig %s destroy",
		tun->devname);

	journal_notice("tun> sys:: %s\n", sys);
	ret = system(sys);

	return ret;
}

extern int tun_up(char *devname, char *addr) 
{
	journal_ftrace(__func__);

	char sys[128];
	int ret;

	snprintf(sys, 128, "ifconfig %s %s link0",
		devname,
		addr);

	journal_notice("tun]> sys:: %s\n", sys);
	ret = system(sys);

	return ret;
}

extern int tun_create(int *devname, int *fd)
{
	char name[255];
	int ret;
	int dev;

	struct tuninfo info;

	for (dev = 255; dev >= 0; (dev)--) {
		snprintf(name, sizeof(name), "/dev/tun%i", dev);
		*fd = open(name, O_RDWR);

		if (*fd >= 0)
			break;
	}

	snprintf(devname, IFNAMSIZ, "%s", name+5);	

	/* Flags given will be set; flags omitted will be cleared; */
	ret = ioctl(*fd, TUNGIFINFO, &info);
	if (ret < 0) {
		journal_notice("tun_tuninfo]> ioctl TUNGIFINFO failed %s :: %s:%i\n", name, __FILE__, __LINE__);
		close(*fd);
		return -1;
	}
	/* Layer 2 tunneling mode */
	info.flags = IFF_UP | IFF_BROADCAST;
	info.type = IFT_ETHER;
	ret = ioctl(*fd, TUNSIFINFO, &info);
	if (ret < 0) {
		journal_notice("tun_tuninfo]> ioctl TUNSIFINFO failed %s :: %s:%i\n", name, __FILE__, __LINE__);
		close(*fd);
		return -1;
	}	

	return 0;
}
