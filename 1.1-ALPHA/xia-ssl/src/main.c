
#include <unistd.h>

#include <xia/event.h>
#include <xia/journal.h>
#include <xia/netbus.h>
#include <xia/sched.h>

#include "ssl.h"

int main(int argc, char *argv[])
{

	int opt, M_FLAG = 0;

	while ((opt = getopt(argc, argv, "cs")) != -1) {
		switch (opt) {
			case 'c':
				M_FLAG = SSL_CLIENT;
				break;
			case 's':
				M_FLAG = SSL_SERVER;
				break;
			default:
				printf("-c, -s\n");
				journal_failure(EXIT_ERR, "getopt() failed :: %s:%i\n", __FILE__, __LINE__);
				_exit(EXIT_ERR);
		}
	}

	if (M_FLAG == 0) {
		printf("-c, -s\n");
		journal_failure(EXIT_ERR, "getopt() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (journal_init()) {
		journal_failure(EXIT_ERR, "ssl]> journal_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (event_init()) {
		journal_failure(EXIT_ERR, "ssl]> event_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (scheduler_init()) {
		journal_failure(EXIT_ERR, "ssl]> scheduler_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (netbus_init()) {
		journal_failure(EXIT_ERR, "ssl]> netbus_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (ssl_init(M_FLAG, "127.0.0.1", 4040)) {
		journal_failure(EXIT_ERR, "ssl]> ssl_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	scheduler();
}

