#ifndef SNIFFER_H
#define SNIFFER_H

#include <pcap.h>

void sniffer();
int sniffer_register(char *, void (*)(u_char *, const struct pcap_pkthdr *, const u_char *));
int sniffer_init();

#endif /* SNIFFER_H */
