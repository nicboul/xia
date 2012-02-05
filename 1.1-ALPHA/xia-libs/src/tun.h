/*
 * See COPYRIGHTS file.
 */


#ifndef XIA_TUN_H
#define XIA_TUN_H

#include "netbus.h"

int tun_up(char *, char *);
int tun_create(int *, int *);
int tun_destroy(iface_t *);

#endif /* XIA_TUN_H */
