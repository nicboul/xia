/*
 * Copyright (c) 2008; Nicolas Bouliane <nicboul@gmail.com>
 */

#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hooklet.h"
#include "journal.h"
#include "utils.h"

static hooklet_t *hooklets[HOOKLET_MAX] = {NULL};
extern void hooklet_show()
{
	journal_strace("hooklet_show");
	
	int i; 
	for (i = 0; i < HOOKLET_MAX; i++) {

		if (hooklets[i]) {
			journal_notice("hooklet]> %s::%i\n", hooklets[i]->name, hooklets[i]->hookin());
		}
	}
}

extern hooklet_t *hooklet_inherit(int hookin)
{
	journal_strace("hooklet_inherit");

	if (hookin < 0 || hookin > HOOKLET_MAX) {
		journal_notice("The hookin is out of range %i"
				" :: %s:%i\n", __FILE__, __LINE__);
		return NULL;
	}

	return hooklets[hookin];
}

extern int hooklet_map_cb(hooklet_t *hk, hooklet_cb_t *cb)
{
	journal_strace("hooklet_map_cb");

	int i, err = 0;
	if (hk->handle == NULL) {
		journal_notice("The hooklet handle is invalid."
				" :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	
	for (i = 0; cb[i].name != NULL; i++) {
		
		*cb[i].ptr = dlsym(hk->handle, cb[i].name);
		if (*cb[i].ptr == NULL) {
			journal_notice("The hooklet `%s` has no callback `%s` implemented."
					" :: %s:%i\n", hk->name, cb[i].name, __FILE__, __LINE__);

			err = -1;
		}
	}

	return err;
}

extern int hooklet_init(char *hooklet_list)
{
	journal_strace("hooklet_init");

	hooklet_t *hk = NULL;
	char path[PATHLEN];
	/* 
	 * s_tk: current token
	 * a_tk: begin a token
	 * z_tk: end a token
	 */
	char *s_tk, *z_tk, *a_tk = hooklet_list;

	if (hooklet_list == NULL) {
		journal_notice("The hooklet list is empty. :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	while ((s_tk = x_strtok(&a_tk, &z_tk, ','))) {
	
		if (hk == NULL)
			hk = calloc(1, sizeof(hooklet_t));	

		hk->name = strdup(trim(s_tk));
		
		snprintf(path, PATHLEN, "%s/%s.so", HOOKLETS_PATH, hk->name);
		hk->handle = dlopen(path, RTLD_GLOBAL|RTLD_NOW);

		if (hk->handle == NULL) {
			journal_notice("Can't load `%s`, dlopen() failed. : %s :: %s:%i\n", \
					hk->name, dlerror(), __FILE__, __LINE__);
			continue;
		}

		hk->hookin = dlsym(hk->handle, "hookin");
		if (hk->hookin == NULL) {
			journal_notice("`%s` failed to return any hookin callback. %s :: %s:%i\n", \
					hk->name, dlerror(), __FILE__, __LINE__);

			continue;
		}

		hooklets[hk->hookin()] = hk;
		hk = NULL;
	}

	if (hk != NULL) {
		free(hk->name); free(hk);
	}

	return 0;
}

