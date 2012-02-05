/*
 * See COPYRIGHTS file.
 */


#ifndef DBAL_H
#define DBAL_H

#include <xia/dnds.h>
#include <xia/utils.h>

int dbal_init(const char *, const char *, const char *, const char *);

/* DNDS_ACL */
DNDS_ACL *dbal_acl_get(DNDS_ACL *);
int dbal_acl_list(DNDS_ACL *, void (*)(DNDS_ACL *));
int dbal_acl_new(DNDS_ACL *);
int dbal_acl_edit(DNDS_ACL *);
int dbal_acl_clear(DNDS_ACL *);
int dbal_acl_delete(DNDS_ACL *);
int dbal_acl_src_group_member(DNDS_ACL *, void (*)(DNDS_ACL_GROUP *));
int dbal_acl_src_group_not_member(DNDS_ACL *, void (*)(DNDS_ACL_GROUP *));
int dbal_acl_src_group_map(DNDS_ACL *, DNDS_ACL_GROUP *);
int dbal_acl_src_group_unmap(DNDS_ACL *, DNDS_ACL_GROUP *);
int dbal_acl_src_host_member(DNDS_ACL *, void (*)(DNDS_HOST *));
int dbal_acl_src_host_not_member(DNDS_ACL *, void (*)(DNDS_HOST *));
int dbal_acl_src_host_map(DNDS_ACL *, DNDS_HOST *);
int dbal_acl_src_host_unmap(DNDS_ACL *, DNDS_HOST *);
int dbal_acl_dst_group_member(DNDS_ACL *, void (*)(DNDS_ACL_GROUP *));
int dbal_acl_dst_group_not_member(DNDS_ACL *, void (*)(DNDS_ACL_GROUP *));
int dbal_acl_dst_group_map(DNDS_ACL *, DNDS_ACL_GROUP *);
int dbal_acl_dst_group_unmap(DNDS_ACL *, DNDS_ACL_GROUP *);
int dbal_acl_dst_host_member(DNDS_ACL *, void (*)(DNDS_HOST *));
int dbal_acl_dst_host_not_member(DNDS_ACL *, void (*)(DNDS_HOST *));
int dbal_acl_dst_host_map(DNDS_ACL *, DNDS_HOST *);
int dbal_acl_dst_host_unmap(DNDS_ACL *, DNDS_HOST *);

/* DNDS_ACL_GROUP */
DNDS_ACL_GROUP *dbal_acl_group_get(DNDS_ACL_GROUP *);
int dbal_acl_group_list(DNDS_ACL_GROUP *, void (*)(DNDS_ACL_GROUP *));
int dbal_acl_group_new(DNDS_ACL_GROUP *);
int dbal_acl_group_edit(DNDS_ACL_GROUP *);
int dbal_acl_group_clear(DNDS_ACL_GROUP *);
int dbal_acl_group_delete(DNDS_ACL_GROUP *);
int dbal_acl_group_host_member(DNDS_ACL_GROUP *, void (*)(DNDS_HOST *));
int dbal_acl_group_host_not_member(DNDS_ACL_GROUP *, void (*)(DNDS_HOST *));
int dbal_acl_group_host_map(DNDS_ACL_GROUP *, DNDS_HOST *);
int dbal_acl_group_host_unmap(DNDS_ACL_GROUP *, DNDS_HOST *);

/* DNDS_ADDR_POOL */
DNDS_ADDR_POOL *dbal_addr_pool_get(DNDS_ADDR_POOL *);
int dbal_addr_pool_list(void (*)(DNDS_ADDR_POOL *));
int dbal_addr_pool_new(DNDS_ADDR_POOL *);
int dbal_addr_pool_edit(DNDS_ADDR_POOL *);
int dbal_addr_pool_clear(DNDS_ADDR_POOL *);
int dbal_addr_pool_delete(DNDS_ADDR_POOL *);

/* DNDS_CONTEXT */
DNDS_CONTEXT *dbal_context_get(DNDS_CONTEXT *);
int dbal_context_list(void (*)(DNDS_CONTEXT *));
int dbal_context_new(DNDS_CONTEXT *);
int dbal_context_edit(DNDS_CONTEXT *);
int dbal_context_clear(DNDS_CONTEXT *);
int dbal_context_delete(DNDS_CONTEXT *);

/* DNDS_HOST */
DNDS_HOST *dbal_host_get(DNDS_HOST *);
int dbal_host_list(DNDS_HOST *, void (*)(DNDS_HOST *));
int dbal_host_new(DNDS_HOST *);
int dbal_host_edit(DNDS_HOST *);
int dbal_host_clear(DNDS_HOST *);
int dbal_host_delete(DNDS_HOST *);

/* DNDS_NODE */
DNDS_NODE *dbal_node_get(DNDS_NODE *);
int dbal_node_list(void (*)(DNDS_NODE *));
int dbal_node_new(DNDS_NODE *);
int dbal_node_edit(DNDS_NODE *);
int dbal_node_clear(DNDS_NODE *);
int dbal_node_delete(DNDS_NODE *);

/* DNDS_PEER */
DNDS_PEER *dbal_peer_get(DNDS_PEER *);
int dbal_peer_list(DNDS_PEER *, void (*)(DNDS_PEER *));
int dbal_peer_new(DNDS_PEER *);
int dbal_peer_edit(DNDS_PEER *);
int dbal_peer_clear(DNDS_PEER *);
int dbal_peer_delete(DNDS_PEER *);
int dbal_peer_user_member(DNDS_PEER *, void (*)(DNDS_USER *));
int dbal_peer_user_not_member(DNDS_PEER *, void (*)(DNDS_USER *));
int dbal_peer_user_map(DNDS_PEER *, DNDS_USER *);
int dbal_peer_user_unmap(DNDS_PEER *, DNDS_USER *);
int dbal_peer_host_member(DNDS_PEER *, void (*)(DNDS_HOST *));
int dbal_peer_host_not_member(DNDS_PEER *, void (*)(DNDS_HOST *));
int dbal_peer_host_map(DNDS_PEER *, DNDS_HOST *);
int dbal_peer_host_unmap(DNDS_PEER *, DNDS_HOST *);

/* DNDS_PERM */
DNDS_PERM *dbal_perm_get(DNDS_PERM *);
DNDS_PERM *dbal_perm_get_by_node(DNDS_NODE *);
int dbal_perm_list(void (*)(DNDS_PERM *));
int dbal_perm_new(DNDS_PERM *);
int dbal_perm_edit(DNDS_PERM *);
int dbal_perm_clear(DNDS_PERM *);
int dbal_perm_delete(DNDS_PERM *);

/* DNDS_USER */
DNDS_USER *dbal_user_get(DNDS_USER *);
int dbal_user_list(DNDS_USER *, void (*)(DNDS_USER *));
int dbal_user_new(DNDS_USER *);
int dbal_user_edit(DNDS_USER *);
int dbal_user_clear(DNDS_USER *);
int dbal_user_delete(DNDS_USER *);

#endif /* DBAL_H */
