#ifndef __HOOKLET_H
#define __HOOKLET_H

typedef struct {
	
	char *name;
	void *handle;
	int (*hookin)();

} hooklet_t;

typedef struct {

	char *name;
	void (**ptr)();

} hooklet_cb_t; 

#define CB(c) ((void (**)())&c)

#ifndef HOOKLETS_PATH
# define HOOKLETS_PATH "/usr/local/libexec/xia/xiad"
#endif

int hooklet_init(char *);
int hooklet_map_cb(hooklet_t *, hooklet_cb_t *);
hooklet_t *hooklet_inherit(int);
void hooklet_show();

enum {
	HOOKLET_ACL = 0,
	HOOKLET_DBAL,
	HOOKLET_MAX	/* This one MUST be the last */
};

#endif
