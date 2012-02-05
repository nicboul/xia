#ifndef __MUXIA_H
#define __MUXIA_H

#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#define muxia_reply_init(mux, msglen) \
	do { \
		memset(&(mux), 0, sizeof(struct muxia_info)); \
		(mux).msgp_size = (msglen); \
	} while (0);

struct muxia_info {
	void *msgp;		/* A pointer to an *_msg struct */
	size_t msgp_size;	/* Size of the message */
	uint8_t ret;		/* Subsystem return value */
};

extern int muxia_init(char *, int);
extern void muxia_register(struct muxia_info *(*)(void *), uint8_t);
extern struct muxia_info *muxia_exec(const uint8_t, void *);

#endif /* __MUXIA_H */
