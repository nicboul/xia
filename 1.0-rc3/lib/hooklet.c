
/*
 * See COPYRIGHTS file.
 */

#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hooklet.h"
#include "journal.h"
#include "utils.h"

static hooklet_t *hooklets[HOOKLET_MAX] = {NULL};

extern void hooklet_show()
{
	journal_ftrace(__func__);
	
	int i; 
	for (i = 0; i < HOOKLET_MAX; i++) {

		if (hooklets[i]) {
			journal_notice("hooklet]> %s::%i\n", hooklets[i]->name, hooklets[i]->hookin());
		}
	}
}

extern hooklet_t *hooklet_inherit(int hookin)
{
	journal_ftrace(__func__);

	if (hookin < 0 || hookin > HOOKLET_MAX) {
		journal_notice("hooklet]> The hookin is out of range %i"
				" :: %s:%i\n", __FILE__, __LINE__);
		return NULL;
	}

	return hooklets[hookin];
}

extern int hooklet_map_cb(hooklet_t *hk, hooklet_cb_t *cb)
{
	journal_ftrace(__func__);

	int i, err = 0;
	if (hk->handle == NULL) {
		journal_notice("hooklet]> The hooklet handle is invalid."
				" :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	
	for (i = 0; cb[i].name != NULL; i++) {
		
		*cb[i].ptr = dlsym(hk->handle, cb[i].name);
		if (*cb[i].ptr == NULL) {
			journal_notice("hooklet]> The hooklet `%s` has no callback `%s` implemented."
					" :: %s:%i\n", hk->name, cb[i].name, __FILE__, __LINE__);

			err = -1;
		}
	}

	return err;
}

extern int hooklet_init(char *hooklet_list, const char *hooklet_path)
{
	journal_ftrace(__func__);

	hooklet_t *hk = NULL;
	char fullname[PATHLEN];
	/* 
	 * s_tk: current token
	 * a_tk: begin a token
	 * z_tk: end a token
	 */
	char *s_tk, *z_tk, *a_tk = hooklet_list;

	if (hooklet_list == NULL) {
		journal_notice("hooklet]> The hooklet list is empty. :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	if (hooklet_path == NULL) {
		journal_notice("hooklet]> The hooklet path is empty. :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	while ((s_tk = x_strtok(&a_tk, &z_tk, ','))) {
	
		if (hk == NULL)
			hk = calloc(1, sizeof(hooklet_t));

		if (hk == NULL) {
			journal_notice("hooklet]> calloc() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
			return -1;
		}

		hk->name = strdup(trim(s_tk));
		
		snprintf(fullname, PATHLEN, "%s/%s.so", hooklet_path, hk->name);
		hk->handle = dlopen(fullname, RTLD_GLOBAL|RTLD_NOW);

		if (hk->handle == NULL) {
			journal_notice("hooklet]> Can't load `%s`, dlopen() : %s :: %s:%i\n", \
					hk->name, dlerror(), __FILE__, __LINE__);
			continue;
		}

		hk->hookin = dlsym(hk->handle, "hookin");
		if (hk->hookin == NULL) {
			journal_notice("hooklet]> `%s` failed to return any hookin callback. %s :: %s:%i\n", \
					hk->name, dlerror(), __FILE__, __LINE__);

			continue;
		}

		hooklets[hk->hookin()] = hk;
		hk = NULL;
	}

	if (hk != NULL) {

		free(hk->name);
		free(hk);
	}

	return 0;
}

