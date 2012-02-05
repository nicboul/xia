
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "event.h"
#include "journal.h"

static event_t *events[EVENT_MAX] = {NULL};
extern int event_register(int EVENT, char *name, void (*cb)(void *), int prio) 
{
    journal_strace("event_register");

    event_t *ev_itr, *ev = malloc(sizeof(event_t));
    ev->name = strdup(name);
    ev->cb = cb;
    ev->prio = prio;

    if (events[EVENT] == NULL) {

	events[EVENT] = ev;
	ev->next = NULL;

    } else if (prio == PRIO_HIGH) {

	ev->next = events[EVENT];
	events[EVENT] = ev;

    } else if (prio == PRIO_LOW) {

	ev_itr = events[EVENT];
	while (ev_itr->next != NULL)
	    ev_itr = ev_itr->next;
	ev_itr->next = ev;
	ev->next = NULL;

    } else if (prio == PRIO_AGNOSTIC) {

	if (events[EVENT]->prio == PRIO_AGNOSTIC) {

	    ev->next = events[EVENT];
	    events[EVENT] = ev;

	} else if (events[EVENT]->prio == PRIO_HIGH) {

	    ev_itr = events[EVENT];
	    while (ev_itr->next != NULL && ev_itr->next->prio == PRIO_HIGH)
		ev_itr = ev_itr->next;
	    ev->next = ev_itr->next;
	    ev_itr = ev;

	} else if (events[EVENT]->prio == PRIO_LOW) {

	    ev->next = events[EVENT];
	    events[EVENT] = ev;
	}
    }

    return 0;
}

extern void event_throw(int EVENT, void *data)
{
	journal_strace("event_throw");

	event_t *ev;
	ev = events[EVENT];
	journal_notice("event]> throw::%i\n", EVENT);

	while (ev) {
		
		journal_notice("event]> %s\n", ev->name);
		ev->cb(data);
		ev = ev->next;
	}
	
	if (data != NULL)
		free(data);
	
	return;
}

static void sig_handler(int signum, siginfo_t *info, void *ucontext)
{  
	journal_strace("sig_handler");

	sigset_t block_mask, old_mask;
	sigfillset(&block_mask);
	sigprocmask(SIG_BLOCK, &block_mask, &old_mask);
	
	switch (signum) {

		case SIGSEGV:
			journal_notice("event]> caught SIGSEGV :: %s:%i\n", __FILE__, __LINE__);
			event_throw(EVENT_EXIT, NULL);
			break;
		case SIGINT:
			journal_notice("event]> caught SIGINT :: %s:%i\n", __FILE__, __LINE__);
			event_throw(EVENT_EXIT, NULL);
			break;

		case SIGALRM:
			event_throw(EVENT_SCHED, NULL);
			break;
	}

	sigprocmask(SIG_SETMASK, &old_mask, NULL);

	return;
}

extern int event_init()
{
	journal_strace("event_init");

	struct sigaction act;
	sigset_t block_mask;

	/* We need a special hander
	 * to catch special signals
	 */
	sigfillset(&block_mask);

	act.sa_sigaction = sig_handler;
	act.sa_mask = block_mask;
	act.sa_flags = 0;

	sigaction(SIGINT, &act, NULL);
	sigaction(SIGALRM, &act, NULL);
	sigaction(SIGIO, &act, NULL);
	sigaction(SIGSEGV, &act, NULL);

	return 0;
}
