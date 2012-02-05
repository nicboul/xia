#ifndef CAPTURE_H
#define CAPTURE_H

#include <pcap.h>

extern int capture_init(const char *);
extern int capture_dhcp_only();
extern void capture_set_handler(pcap_handler);
extern void capture();

#endif /* CAPTURE_H */
