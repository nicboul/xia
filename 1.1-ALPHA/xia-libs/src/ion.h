/*
 * See COPYRIGHTS file.
 */


#ifndef XIA_ION_H
#define XIA_ION_H

/* Input Output Notification */

int ion_poke(int queue, void (*notify)(void *udata, int flag));
int ion_add(int queue, int fd, void *udata);
int ion_new();

#define NUM_EVENTS 64
#define BACKING_STORE 512

#define ION_READ 1
#define ION_WRTE 2
#define ION_EROR 3

#endif /* XIA_ION_H */
