#ifndef DISCOVERY_H
#define DISCOVERY_H

#include "reslist.h"

extern int discovery_set_res_addr(reslist_t *);
extern int discovery_set_res_hostname(reslist_t *);
extern int discovery_set_res_dead(reslist_t *);
extern int discovery_set_res_alive(reslist_t *);

#endif /* DISCOVERY_H */
