
#ifndef __DBAL_H
#define __DBAL_H

#include "../lib/utils.h"

int dbal_init(const char *, const char *, const char *, const char *);

int_vector *dbal_get_token_type(const char *);
int_vector *dbal_get_token_status(const char *);
int_vector *dbal_get_res_membership(const char *);
int_vector *dbal_get_usr_membership(const char *);
int_vector *dbal_get_bridge_owner_context(const char *);

int dbal_set_token_addr_mapping(const char *, const char *);
int dbal_unset_token_addr_mapping(const char *);
int dbal_set_res_addr_mapping(const char *, const char *, const char *);
int dbal_set_res_hostname_mapping(const char *, const char *, const char *);
int dbal_set_res_status(const char *, const char *, const unsigned int);

#endif
