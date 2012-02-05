#ifndef __UTILS_H
#define __UTILS_H

#include <stdint.h>

#define PATHLEN 256

extern int daemonize();

extern int bv_set(const unsigned int, uint8_t *, const unsigned int);
extern int bv_unset(const unsigned int, uint8_t *, const unsigned int);
extern int bv_test(const unsigned int, uint8_t *, const unsigned int);

extern char *trim(char *str);
extern char *x_strtok(char **, char **, char);

extern int swap_context(char *, const unsigned int);
extern char *x509_get_cn(char *);

typedef int int_vector;

#define VECTOR_IDX_SIZE 0
#define VECTOR_IDX_MAX 1
#define VECTOR_IDX_BEGIN 2
#define VECTOR_SET_MAX(n) (n + VECTOR_IDX_BEGIN + VECTOR_IDX_MAX)
#define VECTOR_SET_SIZE(n) (n - VECTOR_IDX_BEGIN)


#define true 1
#define false !true

#endif
