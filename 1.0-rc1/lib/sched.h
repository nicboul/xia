#ifndef __SCHED_H
#define __SCHED_H

void sched_register(int, char *, void (*)(void *), unsigned int, void *udata);
void scheduler();

typedef struct task {

	struct task *next;
	char *name;
	void (*cb)(void *);
	long frequency;
	time_t lastexec;
	void *udata;

} task_t;


enum {
	SCHED_APERIODIC = 0,
	SCHED_PERIODIC,
	SCHED_SPORADIC, 
	SCHED_MAX /* This one MUST be the last */
};

#endif
