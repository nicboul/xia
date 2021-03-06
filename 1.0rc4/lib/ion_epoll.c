
/*
 * See COPYRIGHTS file.
 */

#include <sys/epoll.h>
#include <errno.h>
#include <string.h>

#include "ion.h"
#include "journal.h"

static struct epoll_event ep_ev[NUM_EVENTS];
int ion_poke(int queue, void (*notify)(void *udata, int flag))
{
	journal_ftrace(__func__);

	int nfd, flag, i;

	nfd = epoll_wait(queue, ep_ev, NUM_EVENTS, 1);
	if (nfd < 0) {
		journal_notice("ion]> epoll_wait() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		return -1;
	}

	for (i = 0; i < nfd; i++) {

		flag = ION_EROR;

		if (ep_ev[i].events & EPOLLIN)
			flag = ION_READ;
		if (ep_ev[i].events & EPOLLRDHUP)
			flag = ION_EROR;

		notify(ep_ev[i].data.ptr, flag);
	}

	return nfd;
}

int ion_add(int queue, int fd, void *udata)
{
	journal_ftrace(__func__);

	struct epoll_event nevent;
	int ret;

	nevent.events = EPOLLIN;
	nevent.data.ptr = udata;

	ret = epoll_ctl(queue, EPOLL_CTL_ADD, fd, &nevent);
	if (ret < 0)
		journal_notice("ion]> epoll_ctl() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);

	return ret;
}

int ion_new()
{
	journal_ftrace(__func__);

	int epoll_fd;

	epoll_fd = epoll_create(BACKING_STORE);
	if (epoll_fd <	0)
		journal_notice("ion]> epoll_create() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);

	return epoll_fd;
}

