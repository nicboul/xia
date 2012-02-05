/*
 * See COPYRIGHTS file.
 */


#ifndef XIA_SCHED_H
#define XIA_SCHED_H

extern int scheduler_init();
extern void sched_register(int, char *, void (*)(void *), unsigned int, void *udata);
extern void scheduler();

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

#endif /* XIA_SCHED_H */
