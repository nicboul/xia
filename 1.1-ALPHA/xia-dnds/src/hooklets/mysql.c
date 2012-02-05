/*
 * See COPYRIGHTS file.
 */


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <mysql/mysql.h>
#include <mysql/errmsg.h>

#include <xia/dnds.h>
#include <xia/hooklet.h>
#include <xia/journal.h>
#include <xia/utils.h>

#define HK_MYSQL_QUERY(n, q) static char (n)[sizeof((q))] = (q)

MYSQL mysql;

struct stmt {
	MYSQL_STMT	*stmt;		/* MySQL stmt handler */

	char		*query;		/* SQL query */
	size_t		q_len;		/* SQL query length */

	MYSQL_BIND	*in_buf;	/* MySQL input buffers */
	uint8_t		in_buf_count;	/* number of input buffers */
	MYSQL_BIND	*out_buf;	/* MySQL output buffers */
	uint8_t		out_buf_count;	/* number of output buffers */
};

static inline
void init_input_buffer(struct stmt *s, MYSQL_BIND *buf, int count)
{
	journal_ftrace(__func__);

	s->in_buf = buf;
	s->in_buf_count = count;
	memset(buf, 0, count * sizeof(MYSQL_BIND));
}

static inline
void init_output_buffer(struct stmt *s, MYSQL_BIND *buf, int count)
{
	journal_ftrace(__func__);

	s->out_buf = buf;
	s->out_buf_count = count;
	memset(buf, 0, count * sizeof(MYSQL_BIND));
}

static int hk_mysql_execute(struct stmt *stmt)
{
	journal_ftrace(__func__);

	int ret;

	journal_notice("Running MySQL query\n\n\t%s\n\n", stmt->query);

	ret = mysql_stmt_prepare(stmt->stmt, stmt->query, stmt->q_len);
	if (ret != 0) {
		journal_notice("mysql]> mysql_stmt_prepare failed, %s :: %s:%i\n",
		    mysql_stmt_error(stmt->stmt), __FILE__, __LINE__);
		return -1;
	}

	if (stmt->in_buf) {
		ret = mysql_stmt_bind_param(stmt->stmt, stmt->in_buf);
		if (ret != 0) {
			journal_notice("%s%s :: %s:%i\n",
			    "mysql]> mysql_stmt_bind_param failed, ",
			    mysql_stmt_error(stmt->stmt), __FILE__, __LINE__);
			return -1;
		}

		ret = mysql_stmt_param_count(stmt->stmt);
		if (ret != stmt->in_buf_count) {
			journal_notice("%s :: %s:%i\n",
			    "mysql]> input buffer mismatch",
			    __FILE__, __LINE__);
			return -1;
		}
	}

	if (stmt->out_buf) {
		ret = mysql_stmt_bind_result(stmt->stmt, stmt->out_buf);
		if (ret != 0) {
			journal_notice("%s%s :: %s:%i\n",
			    "mysql]> mysql_stmt_bind_param failed, ",
			    mysql_stmt_error(stmt->stmt), __FILE__, __LINE__);
			return -1;
		}
	}

	ret = mysql_stmt_execute(stmt->stmt);
	if (ret != 0) {
		journal_notice("%s%s :: %s:%i\n",
		    "mysql]> mysql_stmt_execute failed, ",
		    mysql_stmt_error(stmt->stmt), __FILE__, __LINE__);
		return -1;
	}

	return 0;
}

#define ACL_COLUMN "`id`, `context`, `desc`, `age`"
#define ACL_VALUE "?, ?, ?, ?"
#define ACL_TABLE "`acl`"

#define ACL_SRC_MAP_TABLE "`acl_src_map`"
#define ACL_DST_MAP_TABLE "`acl_dst_map`"

static inline
void hk_mysql_bind_acl(MYSQL_BIND *buf, DNDS_ACL *acl)
{
	size_t desc_size;

	desc_size = strlen(acl->desc);
	if (desc_size == 0)
		desc_size = DNDS_ACL_DESC_SIZE;

	buf[DNDS_ACL_ID].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_ACL_ID].is_unsigned = 1;
	buf[DNDS_ACL_ID].buffer = (char *)&(acl->id);
	
	buf[DNDS_ACL_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	buf[DNDS_ACL_CONTEXT].is_unsigned = 1;
	buf[DNDS_ACL_CONTEXT].buffer = (char *)&(acl->context);

	buf[DNDS_ACL_DESC].buffer_type = MYSQL_TYPE_STRING;
	buf[DNDS_ACL_DESC].buffer_length = desc_size;
	buf[DNDS_ACL_DESC].buffer = acl->desc;

	buf[DNDS_ACL_AGE].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_ACL_AGE].is_unsigned = 1;
	buf[DNDS_ACL_AGE].buffer = (char *)&(acl->age);
}

#define ACL_GROUP_COLUMN "`id`, `context`, `name`, `desc`, `age`"
#define ACL_GROUP_VALUE "?, ?, ?, ?, ?"
#define ACL_GROUP_TABLE "`acl_group`"

#define ACL_GROUP_MAP_TABLE "`acl_group_map`"

static inline
void hk_mysql_bind_acl_group(MYSQL_BIND *buf, DNDS_ACL_GROUP *acl_group)
{
	size_t name_size, desc_size;

	name_size = strlen(acl_group->name);
	if (name_size == 0)
		name_size = DNDS_ACL_GROUP_NAME_SIZE;

	desc_size = strlen(acl_group->desc);
	if (desc_size == 0)
		desc_size = DNDS_ACL_GROUP_DESC_SIZE;

	buf[DNDS_ACL_GROUP_ID].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_ACL_GROUP_ID].is_unsigned = 1;
	buf[DNDS_ACL_GROUP_ID].buffer = (char *)&(acl_group->id);

	buf[DNDS_ACL_GROUP_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	buf[DNDS_ACL_GROUP_CONTEXT].is_unsigned = 1;
	buf[DNDS_ACL_GROUP_CONTEXT].buffer = (char *)&(acl_group->context);

	buf[DNDS_ACL_GROUP_NAME].buffer_type = MYSQL_TYPE_STRING;
	buf[DNDS_ACL_GROUP_NAME].buffer_length = name_size;
	buf[DNDS_ACL_GROUP_NAME].buffer = acl_group->name;

	buf[DNDS_ACL_GROUP_DESC].buffer_type = MYSQL_TYPE_STRING;
	buf[DNDS_ACL_GROUP_DESC].buffer_length = desc_size;
	buf[DNDS_ACL_GROUP_NAME].buffer = acl_group->desc;

	buf[DNDS_ACL_GROUP_AGE].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_ACL_GROUP_AGE].is_unsigned = 1;
	buf[DNDS_ACL_GROUP_AGE].buffer = (char *)&(acl_group->age);
}

#define ADDR_POOL_COLUMN "`id`, `local`, `begin`, `end`, " \
			 "`netmask`, `desc`, `age`"
#define ADDR_POOL_VALUE "?, ?, ?, ?, ?, ?, ?"
#define ADDR_POOL_TABLE "`addr_pool`"

static inline
void hk_mysql_bind_addr_pool(MYSQL_BIND *buf, DNDS_ADDR_POOL *addr_pool)
{
	size_t desc_size;

	desc_size = strlen(addr_pool->desc);
	if (desc_size == 0)
		desc_size = DNDS_ADDR_POOL_DESC_SIZE;

	buf[DNDS_ADDR_POOL_ID].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_ADDR_POOL_ID].is_unsigned = 1;
	buf[DNDS_ADDR_POOL_ID].buffer = (char *)&(addr_pool->id);

	buf[DNDS_ADDR_POOL_LOCAL].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_ADDR_POOL_LOCAL].is_unsigned = 1;
	buf[DNDS_ADDR_POOL_LOCAL].buffer = (char *)&(addr_pool->local.s_addr);

	buf[DNDS_ADDR_POOL_BEGIN].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_ADDR_POOL_BEGIN].is_unsigned = 1;
	buf[DNDS_ADDR_POOL_BEGIN].buffer = (char *)&(addr_pool->begin.s_addr);

	buf[DNDS_ADDR_POOL_END].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_ADDR_POOL_END].is_unsigned = 1;
	buf[DNDS_ADDR_POOL_END].buffer = (char *)&(addr_pool->end.s_addr);

	buf[DNDS_ADDR_POOL_NETMASK].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_ADDR_POOL_NETMASK].is_unsigned = 1;
	buf[DNDS_ADDR_POOL_NETMASK].buffer = (char *)&(addr_pool->netmask.s_addr);

	buf[DNDS_ADDR_POOL_DESC].buffer_type = MYSQL_TYPE_STRING;
	buf[DNDS_ADDR_POOL_DESC].buffer_length = desc_size;
	buf[DNDS_ADDR_POOL_DESC].buffer = addr_pool->desc;

	buf[DNDS_ADDR_POOL_AGE].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_ADDR_POOL_AGE].is_unsigned = 1;
	buf[DNDS_ADDR_POOL_AGE].buffer = (char *)&(addr_pool->age);
}

#define CONTEXT_COLUMN "`context`, `dns_zone`, `dns_serial`, " \
		       "`vhost`, `desc`, `age`"
#define CONTEXT_VALUE "?, ?, ?, ?, ?, ?"
#define CONTEXT_TABLE "`context`"

static inline
void hk_mysql_bind_context(MYSQL_BIND *buf, DNDS_CONTEXT *context)
{
	size_t dns_zone_size, vhost_size, desc_size;

	dns_zone_size = strlen(context->dns_zone);
	if (dns_zone_size == 0)
		dns_zone_size = DNDS_CONTEXT_DNS_ZONE_SIZE;

	vhost_size = strlen(context->vhost);
	if (vhost_size == 0)
		vhost_size = DNDS_CONTEXT_VHOST_SIZE;

	desc_size = strlen(context->desc);
	if (desc_size == 0)
		desc_size = DNDS_CONTEXT_DESC_SIZE;

	buf[DNDS_CONTEXT_ID].buffer_type = MYSQL_TYPE_SHORT;
	buf[DNDS_CONTEXT_ID].is_unsigned = 1;
	buf[DNDS_CONTEXT_ID].buffer = (char *)&(context->id);

	buf[DNDS_CONTEXT_DNS_ZONE].buffer_type = MYSQL_TYPE_STRING;
	buf[DNDS_CONTEXT_DNS_ZONE].buffer_length = dns_zone_size;
	buf[DNDS_CONTEXT_DNS_ZONE].buffer = context->dns_zone;

	buf[DNDS_CONTEXT_DNS_SERIAL].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_CONTEXT_DNS_SERIAL].is_unsigned = 1;
	buf[DNDS_CONTEXT_DNS_SERIAL].buffer = (char *)&(context->dns_serial);

	buf[DNDS_CONTEXT_VHOST].buffer_type = MYSQL_TYPE_STRING;
	buf[DNDS_CONTEXT_VHOST].buffer_length = vhost_size;
	buf[DNDS_CONTEXT_VHOST].buffer = context->vhost;

	buf[DNDS_CONTEXT_DESC].buffer_type = MYSQL_TYPE_STRING;
	buf[DNDS_CONTEXT_DESC].buffer_length = desc_size;
	buf[DNDS_CONTEXT_DESC].buffer = context->desc;

	buf[DNDS_CONTEXT_AGE].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_CONTEXT_AGE].is_unsigned = 1;
	buf[DNDS_CONTEXT_AGE].buffer = (char *)&(context->age);
}

#define HOST_COLUMN "`id`, `context`, `peer`, `name`, `haddr`, " \
		    "`addr`, `flag`, `age`"
#define HOST_VALUE "?, ?, ?, ?, ?, ?, ?, ?"
#define HOST_TABLE "`host`"

static inline
void hk_mysql_bind_host(MYSQL_BIND *buf, DNDS_HOST *host)
{
	size_t name_size;

	name_size = strlen(host->name);
	if (name_size == 0)
		name_size = DNDS_HOST_NAME_SIZE;

	buf[DNDS_HOST_ID].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_HOST_ID].is_unsigned = 1;
	buf[DNDS_HOST_ID].buffer = (char *)&(host->id);
	
	buf[DNDS_HOST_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	buf[DNDS_HOST_CONTEXT].is_unsigned = 1;
	buf[DNDS_HOST_CONTEXT].buffer = (char *)&(host->context);

	buf[DNDS_HOST_PEER].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_HOST_PEER].is_unsigned = 1;
	buf[DNDS_HOST_PEER].buffer = (char *)&(host->peer);

	buf[DNDS_HOST_NAME].buffer_type = MYSQL_TYPE_STRING;
	buf[DNDS_HOST_NAME].buffer_length = name_size;
	buf[DNDS_HOST_NAME].buffer = host->name;

	buf[DNDS_HOST_HADDR].buffer_type = MYSQL_TYPE_STRING;
	buf[DNDS_HOST_HADDR].buffer = (char *)&(host->haddr);

	buf[DNDS_HOST_ADDR].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_HOST_ADDR].is_unsigned = 1;
	buf[DNDS_HOST_ADDR].buffer = (char *)&(host->addr.s_addr);

	buf[DNDS_HOST_FLAG].buffer_type = MYSQL_TYPE_TINY;
	buf[DNDS_HOST_FLAG].is_unsigned = 1;
	buf[DNDS_HOST_FLAG].buffer = &(host->flag);

	buf[DNDS_HOST_AGE].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_HOST_AGE].is_unsigned = 1;
	buf[DNDS_HOST_AGE].buffer = (char *)&(host->age);
}

#define NODE_COLUMN "`id`, `type`, `name`, `addr`, `perm`, `flag`, `age`"
#define NODE_VALUE "?, ?, ?, ?, ?, ?, ?"
#define NODE_TABLE "`node`"

static inline
void hk_mysql_bind_node(MYSQL_BIND *buf, DNDS_NODE *node)
{
	size_t name_size;

	name_size = strlen(node->name);
	if (name_size == 0)
		name_size = DNDS_NODE_NAME_SIZE;

	buf[DNDS_NODE_ID].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_NODE_ID].is_unsigned = 1;
	buf[DNDS_NODE_ID].buffer = (char *)&(node->id);
	
	buf[DNDS_NODE_TYPE].buffer_type = MYSQL_TYPE_TINY;
	buf[DNDS_NODE_TYPE].is_unsigned = 1;
	buf[DNDS_NODE_TYPE].buffer = &(node->type);

	buf[DNDS_NODE_NAME].buffer_type = MYSQL_TYPE_STRING;
	buf[DNDS_NODE_NAME].buffer_length = name_size;
	buf[DNDS_NODE_NAME].buffer = node->name;

	buf[DNDS_NODE_ADDR].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_NODE_ADDR].is_unsigned = 1;
	buf[DNDS_NODE_ADDR].buffer = (char *)&(node->addr.s_addr);

	buf[DNDS_NODE_PERM].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_NODE_PERM].is_unsigned = 1;
	buf[DNDS_NODE_PERM].buffer = (char *)&(node->perm);

	buf[DNDS_NODE_FLAG].buffer_type = MYSQL_TYPE_TINY;
	buf[DNDS_NODE_FLAG].is_unsigned = 1;
	buf[DNDS_NODE_FLAG].buffer = &(node->flag);

	buf[DNDS_NODE_AGE].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_NODE_AGE].is_unsigned = 1;
	buf[DNDS_NODE_AGE].buffer = (char *)&(node->age);
}

#define PEER_COLUMN "`id`, `context`, `name`, `addr`, `flag`, `age`"
#define PEER_VALUE "?, ?, ?, ?, ?, ?"
#define PEER_TABLE "`peer`"

#define PEER_USER_MAP_TABLE "`peer_user_map`"

static inline
void hk_mysql_bind_peer(MYSQL_BIND *buf, DNDS_PEER *peer)
{
	size_t name_size;

	name_size = strlen(peer->name);
	if (name_size == 0)
		name_size = DNDS_PEER_NAME_SIZE;

	buf[DNDS_PEER_ID].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_PEER_ID].is_unsigned = 1;
	buf[DNDS_PEER_ID].buffer = (char *)&(peer->id);
	
	buf[DNDS_PEER_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	buf[DNDS_PEER_CONTEXT].is_unsigned = 1;
	buf[DNDS_PEER_CONTEXT].buffer = (char *)&(peer->context);

	buf[DNDS_PEER_NAME].buffer_type = MYSQL_TYPE_STRING;
	buf[DNDS_PEER_NAME].buffer_length = name_size;
	buf[DNDS_PEER_NAME].buffer = peer->name;

	buf[DNDS_PEER_ADDR].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_PEER_ADDR].is_unsigned = 1;
	buf[DNDS_PEER_ADDR].buffer = (char *)&(peer->addr.s_addr);

	buf[DNDS_PEER_FLAG].buffer_type = MYSQL_TYPE_TINY;
	buf[DNDS_PEER_FLAG].is_unsigned = 1;
	buf[DNDS_PEER_FLAG].buffer = &(peer->flag);

	buf[DNDS_PEER_AGE].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_PEER_AGE].is_unsigned = 1;
	buf[DNDS_PEER_AGE].buffer = (char *)&(peer->age);
}

#define PERM_COLUMN "`id`, `name`, `matrix`, `age`"
#define PERM_VALUE "?, ?, ?, ?"
#define PERM_TABLE "`perm`"

static inline
void hk_mysql_bind_perm(MYSQL_BIND *buf, DNDS_PERM *perm)
{
	size_t name_size;

	name_size = strlen(perm->name);
	if (name_size == 0)
		name_size = DNDS_PERM_NAME_SIZE;

	buf[DNDS_PERM_ID].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_PERM_ID].is_unsigned = 1;
	buf[DNDS_PERM_ID].buffer = (char *)&(perm->id);

	buf[DNDS_PERM_NAME].buffer_type = MYSQL_TYPE_STRING;
	buf[DNDS_PERM_NAME].buffer_length = name_size;
	buf[DNDS_PERM_NAME].buffer = perm->name;

	buf[DNDS_PERM_MATRIX].buffer_type = MYSQL_TYPE_STRING;
	buf[DNDS_PERM_MATRIX].buffer_length = DNDS_PERM_MATRIX_MAX;
	buf[DNDS_PERM_MATRIX].buffer = (char *)(perm->matrix);

	buf[DNDS_PERM_AGE].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_PERM_AGE].is_unsigned = 1;
	buf[DNDS_PERM_AGE].buffer = (char *)&(perm->age);
}

#define USER_COLUMN "`id`, `context`, `name`, `password`, `firstname`, " \
		    "`lastname`, `email`, `role`, `flag`, `age`"
#define USER_VALUE "?, ?, ?, ?, ?, ?, ?, ?, ?, ?"
#define USER_TABLE "`user`"

static inline
void hk_mysql_bind_user(MYSQL_BIND *buf, DNDS_USER *user)
{
	size_t name_size, password_size,
	    firstname_size, lastname_size, email_size;

	name_size = strlen(user->name);
	if (name_size == 0)
		name_size = DNDS_USER_NAME_SIZE;

	password_size = strlen(user->password);
	if (password_size == 0)
		password_size = DNDS_USER_PASSWORD_SIZE;

	firstname_size = strlen(user->firstname);
	if (firstname_size == 0)
		firstname_size = DNDS_USER_FIRSTNAME_SIZE;

	lastname_size = strlen(user->lastname);
	if (lastname_size == 0)
		lastname_size = DNDS_USER_LASTNAME_SIZE;

	email_size = strlen(user->email);
	if (email_size == 0)
		email_size = DNDS_USER_EMAIL_SIZE;

	buf[DNDS_USER_ID].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_USER_ID].is_unsigned = 1;
	buf[DNDS_USER_ID].buffer = (char *)&(user->id);

	buf[DNDS_USER_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	buf[DNDS_USER_CONTEXT].is_unsigned = 1;
	buf[DNDS_USER_CONTEXT].buffer = (char *)&(user->context);

	buf[DNDS_USER_NAME].buffer_type = MYSQL_TYPE_STRING;
	buf[DNDS_USER_NAME].buffer_length = name_size;
	buf[DNDS_USER_NAME].buffer = user->name;

	buf[DNDS_USER_PASSWORD].buffer_type = MYSQL_TYPE_STRING;
	buf[DNDS_USER_PASSWORD].buffer_length = password_size;
	buf[DNDS_USER_PASSWORD].buffer = (char *)&(user->password);

	buf[DNDS_USER_FIRSTNAME].buffer_type = MYSQL_TYPE_STRING;
	buf[DNDS_USER_FIRSTNAME].buffer_length = firstname_size;
	buf[DNDS_USER_FIRSTNAME].buffer = user->firstname;

	buf[DNDS_USER_LASTNAME].buffer_type = MYSQL_TYPE_STRING;
	buf[DNDS_USER_LASTNAME].buffer_length = lastname_size;
	buf[DNDS_USER_LASTNAME].buffer = user->lastname;

	buf[DNDS_USER_EMAIL].buffer_type = MYSQL_TYPE_STRING;
	buf[DNDS_USER_EMAIL].buffer_length = email_size;
	buf[DNDS_USER_EMAIL].buffer = user->email;

	buf[DNDS_USER_ROLE].buffer_type = MYSQL_TYPE_TINY;
	buf[DNDS_USER_ROLE].is_unsigned = 1;
	buf[DNDS_USER_ROLE].buffer = &(user->role);

	buf[DNDS_USER_FLAG].buffer_type = MYSQL_TYPE_TINY;
	buf[DNDS_USER_FLAG].is_unsigned = 1;
	buf[DNDS_USER_FLAG].buffer = &(user->flag);

	buf[DNDS_USER_AGE].buffer_type = MYSQL_TYPE_LONG;
	buf[DNDS_USER_AGE].is_unsigned = 1;
	buf[DNDS_USER_AGE].buffer = (char *)&(user->age);
}

/* Virtual functions */

/*
 * acl_get
 */
#define ACL_GET \
\
	"SELECT " ACL_COLUMN " FROM " ACL_TABLE \
	" WHERE `id` = ? AND `context` = ?"

enum {
	INPUT_ACL_GET_ID = 0,
	INPUT_ACL_GET_CONTEXT,
	SIZEOF_INPUT_ACL_GET
};

DNDS_ACL *acl_get(DNDS_ACL *acl)
{
	journal_ftrace(__func__);

	DNDS_ACL *res;

	HK_MYSQL_QUERY(acl_get, ACL_GET);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_GET];
	MYSQL_BIND out[END_DNDS_ACL];
	struct stmt stmt;
	int ret = 0;

	res = malloc(sizeof(DNDS_ACL));
	if (res == NULL)
		return NULL;

	memset(res, 0, sizeof(DNDS_ACL));
	memset(&stmt, 0, sizeof(struct stmt));

	stmt.query = acl_get;
	stmt.q_len = sizeof(ACL_GET);

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_GET);
	in[INPUT_ACL_GET_ID].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_GET_ID].is_unsigned = 1;
	in[INPUT_ACL_GET_ID].buffer = (char *)&(acl->id);

	in[INPUT_ACL_GET_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_ACL_GET_CONTEXT].is_unsigned = 1;
	in[INPUT_ACL_GET_CONTEXT].buffer = (char *)&(acl->context);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_ACL);
	hk_mysql_bind_acl(out, res);

	stmt.stmt = mysql_stmt_init(&mysql);
	if (hk_mysql_execute(&stmt)
	    || (ret = mysql_stmt_fetch(stmt.stmt))) {
		if (ret)
			journal_notice("%s%i :: %s:%i\n",
			    "mysql_stmt_fetch returned ",
			    ret, __FILE__, __LINE__);
		mysql_stmt_close(stmt.stmt);
		free(res);
		return NULL;
	}

	mysql_stmt_close(stmt.stmt);
	return res;
}

/*
 * acl_list
 */
#define ACL_LIST \
\
	"SELECT " ACL_COLUMN " FROM " ACL_TABLE \
	" WHERE `context` = ?"

enum {
	INPUT_ACL_LIST_CONTEXT = 0,
	SIZEOF_INPUT_ACL_LIST
};

int acl_list(DNDS_ACL *acl, void (*cb)(DNDS_ACL *))
{
	journal_ftrace(__func__);

	DNDS_ACL *res;

	HK_MYSQL_QUERY(acl_list, ACL_LIST);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_LIST];
	MYSQL_BIND out[END_DNDS_ACL];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_ACL));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_ACL));
	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_LIST);
	in[INPUT_ACL_LIST_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_ACL_LIST_CONTEXT].is_unsigned = 1;
	in[INPUT_ACL_LIST_CONTEXT].buffer = (char *)&(acl->context);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_ACL);
	hk_mysql_bind_acl(out, res);

	stmt.query = acl_list;
	stmt.q_len = sizeof(ACL_LIST);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * acl_new
 */
#define ACL_NEW \
\
	"INSERT INTO " ACL_TABLE " ( " ACL_COLUMN \
	" ) VALUES ( " ACL_VALUE " )"

int acl_new(DNDS_ACL *acl)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(acl_new, ACL_NEW);
	MYSQL_BIND in[END_DNDS_ACL];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, END_DNDS_ACL);
	hk_mysql_bind_acl(in, acl);

	stmt.query = acl_new;
	stmt.q_len = sizeof(ACL_NEW);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * acl_edit
 */
int acl_edit(DNDS_ACL *acl)
{
	/* XXX */
}

/*
 * acl_clear
 */
int acl_clear(DNDS_ACL *acl)
{
	/* XXX */
}

/*
 * acl_delete
 */
#define ACL_DELETE \
\
	"DELETE FROM " ACL_TABLE \
	" WHERE `id` = ? AND `context` = ?"

enum {
	INPUT_ACL_DELETE_ID = 0,
	INPUT_ACL_DELETE_CONTEXT,
	SIZEOF_INPUT_ACL_DELETE
};

int acl_delete(DNDS_ACL *acl)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(acl_delete, ACL_DELETE);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_DELETE];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_DELETE);
	in[INPUT_ACL_DELETE_ID].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_DELETE_ID].is_unsigned = 1;
	in[INPUT_ACL_DELETE_ID].buffer = (char *)&(acl->id);

	in[INPUT_ACL_DELETE_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_ACL_DELETE_CONTEXT].is_unsigned = 1;
	in[INPUT_ACL_DELETE_CONTEXT].buffer = (char *)&(acl->context);

	stmt.query = acl_delete;
	stmt.q_len = sizeof(ACL_DELETE);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * acl_src_group_member
 */
#define ACL_SRC_GROUP_MEMBER \
\
	"SELECT " ACL_GROUP_COLUMN \
	" FROM " ACL_GROUP_TABLE " WHERE `context` = ?" \
	" AND `id` = ANY (SELECT `acl_group` FROM " \
	ACL_SRC_MAP_TABLE " WHERE `acl` = ?)"

enum {
	INPUT_ACL_SRC_GROUP_MEMBER_CONTEXT = 0,
	INPUT_ACL_SRC_GROUP_MEMBER_ACL,
	SIZEOF_INPUT_ACL_SRC_GROUP_MEMBER
};

int acl_src_group_member(DNDS_ACL *acl, void (*cb)(DNDS_ACL_GROUP *))
{
	journal_ftrace(__func__);

	DNDS_ACL_GROUP *res;

	HK_MYSQL_QUERY(acl_src_group_member, ACL_SRC_GROUP_MEMBER);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_SRC_GROUP_MEMBER];
	MYSQL_BIND out[END_DNDS_ACL_GROUP];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_ACL_GROUP));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_ACL_GROUP));
	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_SRC_GROUP_MEMBER);
	in[INPUT_ACL_SRC_GROUP_MEMBER_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_ACL_SRC_GROUP_MEMBER_CONTEXT].is_unsigned = 1;
	in[INPUT_ACL_SRC_GROUP_MEMBER_CONTEXT].buffer = (char *)&(acl->context);

	in[INPUT_ACL_SRC_GROUP_MEMBER_ACL].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_SRC_GROUP_MEMBER_ACL].is_unsigned = 1;
	in[INPUT_ACL_SRC_GROUP_MEMBER_ACL].buffer = (char *)&(acl->id);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_ACL_GROUP);
	hk_mysql_bind_acl_group(out, res);

	stmt.query = acl_src_group_member;
	stmt.q_len = sizeof(ACL_SRC_GROUP_MEMBER);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * acl_src_group_not_member
 */
#define ACL_SRC_GROUP_NOT_MEMBER \
\
	"SELECT " ACL_GROUP_COLUMN \
	" FROM " ACL_GROUP_TABLE " WHERE `context` = ?" \
	" AND `id` = NOT IN (SELECT `acl_group` FROM " \
	ACL_SRC_MAP_TABLE " WHERE `acl` = ?)"

enum {
	INPUT_ACL_SRC_GROUP_NOT_MEMBER_CONTEXT = 0,
	INPUT_ACL_SRC_GROUP_NOT_MEMBER_ACL,
	SIZEOF_INPUT_ACL_SRC_GROUP_NOT_MEMBER
};

int acl_src_group_not_member(DNDS_ACL *acl, void (*cb)(DNDS_ACL_GROUP *))
{
	journal_ftrace(__func__);

	DNDS_ACL_GROUP *res;

	HK_MYSQL_QUERY(acl_src_group_not_member, ACL_SRC_GROUP_NOT_MEMBER);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_SRC_GROUP_NOT_MEMBER];
	MYSQL_BIND out[END_DNDS_ACL_GROUP];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_ACL_GROUP));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_ACL_GROUP));
	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_SRC_GROUP_NOT_MEMBER);
	in[INPUT_ACL_SRC_GROUP_NOT_MEMBER_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_ACL_SRC_GROUP_NOT_MEMBER_CONTEXT].is_unsigned = 1;
	in[INPUT_ACL_SRC_GROUP_NOT_MEMBER_CONTEXT].buffer = (char *)&(acl->context);

	in[INPUT_ACL_SRC_GROUP_NOT_MEMBER_ACL].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_SRC_GROUP_NOT_MEMBER_ACL].is_unsigned = 1;
	in[INPUT_ACL_SRC_GROUP_NOT_MEMBER_ACL].buffer = (char *)&(acl->id);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_ACL_GROUP);
	hk_mysql_bind_acl_group(out, res);

	stmt.query = acl_src_group_not_member;
	stmt.q_len = sizeof(ACL_SRC_GROUP_NOT_MEMBER);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * acl_src_group_map
 */
#define ACL_SRC_GROUP_MAP \
\
	"INSERT INTO " ACL_SRC_MAP_TABLE \
	" (`acl`, `acl_group`) VALUES (?, ?)"

enum {
	INPUT_ACL_SRC_GROUP_MAP_ACL = 0,
	INPUT_ACL_SRC_GROUP_MAP_GROUP,
	SIZEOF_INPUT_ACL_SRC_GROUP_MAP
};

int acl_src_group_map(DNDS_ACL *acl, DNDS_ACL_GROUP *acl_group)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(acl_src_group_map, ACL_SRC_GROUP_MAP);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_SRC_GROUP_MAP];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_SRC_GROUP_MAP);
	in[INPUT_ACL_SRC_GROUP_MAP_ACL].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_SRC_GROUP_MAP_ACL].is_unsigned = 1;
	in[INPUT_ACL_SRC_GROUP_MAP_ACL].buffer = (char *)&(acl->id);

	in[INPUT_ACL_SRC_GROUP_MAP_GROUP].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_SRC_GROUP_MAP_GROUP].is_unsigned = 1;
	in[INPUT_ACL_SRC_GROUP_MAP_GROUP].buffer = (char *)&(acl_group->id);

	stmt.query = acl_src_group_map;
	stmt.q_len = sizeof(ACL_SRC_GROUP_MAP);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * acl_src_group_unmap
 */
#define ACL_SRC_GROUP_UNMAP \
\
	"DELETE FROM " ACL_SRC_MAP_TABLE \
	" WHERE `acl` = ? AND `acl_group` = ?"

enum {
	INPUT_ACL_SRC_GROUP_UNMAP_ACL = 0,
	INPUT_ACL_SRC_GROUP_UNMAP_GROUP,
	SIZEOF_INPUT_ACL_SRC_GROUP_UNMAP
};

int acl_src_group_unmap(DNDS_ACL *acl, DNDS_ACL_GROUP *acl_group)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(acl_src_group_unmap, ACL_SRC_GROUP_UNMAP);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_SRC_GROUP_UNMAP];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_SRC_GROUP_UNMAP);
	in[INPUT_ACL_SRC_GROUP_UNMAP_ACL].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_SRC_GROUP_UNMAP_ACL].is_unsigned = 1;
	in[INPUT_ACL_SRC_GROUP_UNMAP_ACL].buffer = (char *)&(acl->id);

	in[INPUT_ACL_SRC_GROUP_UNMAP_GROUP].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_SRC_GROUP_UNMAP_GROUP].is_unsigned = 1;
	in[INPUT_ACL_SRC_GROUP_UNMAP_GROUP].buffer = (char *)&(acl_group->id);

	stmt.query = acl_src_group_unmap;
	stmt.q_len = sizeof(ACL_SRC_GROUP_UNMAP);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * acl_src_host_member
 */
#define ACL_SRC_HOST_MEMBER \
\
	"SELECT " HOST_COLUMN " FROM " HOST_TABLE \
	" WHERE `context` = ? AND `id` = ANY" \
	" (SELECT `host` FROM " ACL_SRC_MAP_TABLE " WHERE `acl` = ?)"

enum {
	INPUT_ACL_SRC_HOST_MEMBER_CONTEXT = 0,
	INPUT_ACL_SRC_HOST_MEMBER_ACL,
	SIZEOF_INPUT_ACL_SRC_HOST_MEMBER
};

int acl_src_host_member(DNDS_ACL *acl, void (*cb)(DNDS_HOST *))
{
	journal_ftrace(__func__);

	DNDS_HOST *res;

	HK_MYSQL_QUERY(acl_src_host_member, ACL_SRC_HOST_MEMBER);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_SRC_HOST_MEMBER];
	MYSQL_BIND out[END_DNDS_HOST];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_HOST));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_HOST));
	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_SRC_HOST_MEMBER);
	in[INPUT_ACL_SRC_HOST_MEMBER_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_ACL_SRC_HOST_MEMBER_CONTEXT].is_unsigned = 1;
	in[INPUT_ACL_SRC_HOST_MEMBER_CONTEXT].buffer = (char *)&(acl->context);

	in[INPUT_ACL_SRC_HOST_MEMBER_ACL].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_SRC_HOST_MEMBER_ACL].is_unsigned = 1;
	in[INPUT_ACL_SRC_HOST_MEMBER_ACL].buffer = (char *)&(acl->id);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_HOST);
	hk_mysql_bind_host(out, res);

	stmt.query = acl_src_host_member;
	stmt.q_len = sizeof(ACL_SRC_HOST_MEMBER);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * acl_src_host_not_member
 */
#define ACL_SRC_HOST_NOT_MEMBER \
\
	"SELECT " HOST_COLUMN " FROM " HOST_TABLE \
	" WHERE `context` = ? AND `id` = NOT IN" \
	" (SELECT `host` FROM " ACL_SRC_MAP_TABLE " WHERE `acl` = ?)"

enum {
	INPUT_ACL_SRC_HOST_NOT_MEMBER_CONTEXT = 0,
	INPUT_ACL_SRC_HOST_NOT_MEMBER_ACL,
	SIZEOF_INPUT_ACL_SRC_HOST_NOT_MEMBER
};

int acl_src_host_not_member(DNDS_ACL *acl, void (*cb)(DNDS_HOST *))
{
	journal_ftrace(__func__);

	DNDS_HOST *res;

	HK_MYSQL_QUERY(acl_src_host_not_member, ACL_SRC_HOST_NOT_MEMBER);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_SRC_HOST_NOT_MEMBER];
	MYSQL_BIND out[END_DNDS_HOST];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_HOST));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_HOST));
	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_SRC_HOST_NOT_MEMBER);
	in[INPUT_ACL_SRC_HOST_NOT_MEMBER_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_ACL_SRC_HOST_NOT_MEMBER_CONTEXT].is_unsigned = 1;
	in[INPUT_ACL_SRC_HOST_NOT_MEMBER_CONTEXT].buffer = (char *)&(acl->context);

	in[INPUT_ACL_SRC_HOST_NOT_MEMBER_ACL].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_SRC_HOST_NOT_MEMBER_ACL].is_unsigned = 1;
	in[INPUT_ACL_SRC_HOST_NOT_MEMBER_ACL].buffer = (char *)&(acl->id);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_HOST);
	hk_mysql_bind_host(out, res);

	stmt.query = acl_src_host_not_member;
	stmt.q_len = sizeof(ACL_SRC_HOST_NOT_MEMBER);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * acl_src_host_map
 */
#define ACL_SRC_HOST_MAP \
\
	"INSERT INTO " ACL_SRC_MAP_TABLE \
	" (`acl`, `host`) VALUES (?, ?)"

enum {
	INPUT_ACL_SRC_HOST_MAP_ACL = 0,
	INPUT_ACL_SRC_HOST_MAP_HOST,
	SIZEOF_INPUT_ACL_SRC_HOST_MAP
};

int acl_src_host_map(DNDS_ACL *acl, DNDS_HOST *host)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(acl_src_host_map, ACL_SRC_HOST_MAP);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_SRC_HOST_MAP];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_SRC_HOST_MAP);
	in[INPUT_ACL_SRC_HOST_MAP_ACL].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_SRC_HOST_MAP_ACL].is_unsigned = 1;
	in[INPUT_ACL_SRC_HOST_MAP_ACL].buffer = (char *)&(acl->id);

	in[INPUT_ACL_SRC_HOST_MAP_HOST].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_SRC_HOST_MAP_HOST].is_unsigned = 1;
	in[INPUT_ACL_SRC_HOST_MAP_HOST].buffer = (char *)&(host->id);

	stmt.query = acl_src_host_map;
	stmt.q_len = sizeof(ACL_SRC_HOST_MAP);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * acl_src_host_unmap
 */
#define ACL_SRC_HOST_UNMAP \
\
	"DELETE FROM " ACL_SRC_MAP_TABLE \
	" WHERE `acl` = ? AND `host` = ?"

enum {
	INPUT_ACL_SRC_HOST_UNMAP_ACL = 0,
	INPUT_ACL_SRC_HOST_UNMAP_HOST,
	SIZEOF_INPUT_ACL_SRC_HOST_UNMAP
};

int acl_src_host_unmap(DNDS_ACL *acl, DNDS_HOST *host)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(acl_src_host_unmap, ACL_SRC_HOST_UNMAP);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_SRC_HOST_UNMAP];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_SRC_HOST_UNMAP);
	in[INPUT_ACL_SRC_HOST_UNMAP_ACL].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_SRC_HOST_UNMAP_ACL].is_unsigned = 1;
	in[INPUT_ACL_SRC_HOST_UNMAP_ACL].buffer = (char *)&(acl->id);

	in[INPUT_ACL_SRC_HOST_UNMAP_HOST].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_SRC_HOST_UNMAP_HOST].is_unsigned = 1;
	in[INPUT_ACL_SRC_HOST_UNMAP_HOST].buffer = (char *)&(host->id);

	stmt.query = acl_src_host_unmap;
	stmt.q_len = sizeof(ACL_SRC_HOST_UNMAP);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * acl_dst_group_member
 */
#define ACL_DST_GROUP_MEMBER \
\
	"SELECT " ACL_GROUP_COLUMN \
	" FROM " ACL_GROUP_TABLE " WHERE `context` = ?" \
	" AND `id` = ANY (SELECT `acl_group` FROM " \
	ACL_DST_MAP_TABLE " WHERE `acl` = ?)"

enum {
	INPUT_ACL_DST_GROUP_MEMBER_CONTEXT = 0,
	INPUT_ACL_DST_GROUP_MEMBER_ACL,
	SIZEOF_INPUT_ACL_DST_GROUP_MEMBER
};

int acl_dst_group_member(DNDS_ACL *acl, void (*cb)(DNDS_ACL_GROUP *))
{
	journal_ftrace(__func__);

	DNDS_ACL_GROUP *res;

	HK_MYSQL_QUERY(acl_dst_group_member, ACL_DST_GROUP_MEMBER);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_DST_GROUP_MEMBER];
	MYSQL_BIND out[END_DNDS_ACL_GROUP];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_ACL_GROUP));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_ACL_GROUP));
	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_DST_GROUP_MEMBER);
	in[INPUT_ACL_DST_GROUP_MEMBER_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_ACL_DST_GROUP_MEMBER_CONTEXT].is_unsigned = 1;
	in[INPUT_ACL_DST_GROUP_MEMBER_CONTEXT].buffer = (char *)&(acl->context);

	in[INPUT_ACL_DST_GROUP_MEMBER_ACL].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_DST_GROUP_MEMBER_ACL].is_unsigned = 1;
	in[INPUT_ACL_DST_GROUP_MEMBER_ACL].buffer = (char *)&(acl->id);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_ACL_GROUP);
	hk_mysql_bind_acl_group(out, res);

	stmt.query = acl_dst_group_member;
	stmt.q_len = sizeof(ACL_DST_GROUP_MEMBER);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * acl_dst_group_not_member
 */
#define ACL_DST_GROUP_NOT_MEMBER \
\
	"SELECT " ACL_GROUP_COLUMN \
	" FROM " ACL_GROUP_TABLE " WHERE `context` = ?" \
	" AND `id` = NOT IN (SELECT `acl_group` FROM " \
	ACL_DST_MAP_TABLE " WHERE `acl` = ?)"

enum {
	INPUT_ACL_DST_GROUP_NOT_MEMBER_CONTEXT = 0,
	INPUT_ACL_DST_GROUP_NOT_MEMBER_ACL,
	SIZEOF_INPUT_ACL_DST_GROUP_NOT_MEMBER
};

int acl_dst_group_not_member(DNDS_ACL *acl, void (*cb)(DNDS_ACL_GROUP *))
{
	journal_ftrace(__func__);

	DNDS_ACL_GROUP *res;

	HK_MYSQL_QUERY(acl_dst_group_not_member, ACL_DST_GROUP_NOT_MEMBER);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_DST_GROUP_NOT_MEMBER];
	MYSQL_BIND out[END_DNDS_ACL_GROUP];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_ACL_GROUP));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_ACL_GROUP));
	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_DST_GROUP_NOT_MEMBER);
	in[INPUT_ACL_DST_GROUP_NOT_MEMBER_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_ACL_DST_GROUP_NOT_MEMBER_CONTEXT].is_unsigned = 1;
	in[INPUT_ACL_DST_GROUP_NOT_MEMBER_CONTEXT].buffer = (char *)&(acl->context);

	in[INPUT_ACL_DST_GROUP_NOT_MEMBER_ACL].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_DST_GROUP_NOT_MEMBER_ACL].is_unsigned = 1;
	in[INPUT_ACL_DST_GROUP_NOT_MEMBER_ACL].buffer = (char *)&(acl->id);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_ACL_GROUP);
	hk_mysql_bind_acl_group(out, res);

	stmt.query = acl_dst_group_not_member;
	stmt.q_len = sizeof(ACL_DST_GROUP_NOT_MEMBER);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * acl_dst_group_map
 */
#define ACL_DST_GROUP_MAP \
\
	"INSERT INTO " ACL_DST_MAP_TABLE \
	" (`acl`, `acl_group`) VALUES (?, ?)"

enum {
	INPUT_ACL_DST_GROUP_MAP_ACL = 0,
	INPUT_ACL_DST_GROUP_MAP_GROUP,
	SIZEOF_INPUT_ACL_DST_GROUP_MAP
};

int acl_dst_group_map(DNDS_ACL *acl, DNDS_ACL_GROUP *acl_group)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(acl_dst_group_map, ACL_DST_GROUP_MAP);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_DST_GROUP_MAP];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_DST_GROUP_MAP);
	in[INPUT_ACL_DST_GROUP_MAP_ACL].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_DST_GROUP_MAP_ACL].is_unsigned = 1;
	in[INPUT_ACL_DST_GROUP_MAP_ACL].buffer = (char *)&(acl->id);

	in[INPUT_ACL_DST_GROUP_MAP_GROUP].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_DST_GROUP_MAP_GROUP].is_unsigned = 1;
	in[INPUT_ACL_DST_GROUP_MAP_GROUP].buffer = (char *)&(acl_group->id);

	stmt.query = acl_dst_group_map;
	stmt.q_len = sizeof(ACL_DST_GROUP_MAP);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * acl_dst_group_unmap
 */
#define ACL_DST_GROUP_UNMAP \
\
	"DELETE FROM " ACL_DST_MAP_TABLE \
	" WHERE `acl` = ? AND `acl_group` = ?"

enum {
	INPUT_ACL_DST_GROUP_UNMAP_ACL = 0,
	INPUT_ACL_DST_GROUP_UNMAP_GROUP,
	SIZEOF_INPUT_ACL_DST_GROUP_UNMAP
};


int acl_dst_group_unmap(DNDS_ACL *acl, DNDS_ACL_GROUP *acl_group)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(acl_dst_group_unmap, ACL_DST_GROUP_UNMAP);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_DST_GROUP_UNMAP];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_DST_GROUP_UNMAP);
	in[INPUT_ACL_DST_GROUP_UNMAP_ACL].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_DST_GROUP_UNMAP_ACL].is_unsigned = 1;
	in[INPUT_ACL_DST_GROUP_UNMAP_ACL].buffer = (char *)&(acl->id);

	in[INPUT_ACL_DST_GROUP_UNMAP_GROUP].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_DST_GROUP_UNMAP_GROUP].is_unsigned = 1;
	in[INPUT_ACL_DST_GROUP_UNMAP_GROUP].buffer = (char *)&(acl_group->id);

	stmt.query = acl_dst_group_unmap;
	stmt.q_len = sizeof(ACL_DST_GROUP_UNMAP);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * acl_dst_host_member
 */
#define ACL_DST_HOST_MEMBER \
\
	"SELECT " HOST_COLUMN " FROM " HOST_TABLE \
	" WHERE `context` = ? AND `id` = ANY" \
	" (SELECT `host` FROM " ACL_DST_MAP_TABLE " WHERE `acl` = ?)"

enum {
	INPUT_ACL_DST_HOST_MEMBER_CONTEXT = 0,
	INPUT_ACL_DST_HOST_MEMBER_ACL,
	SIZEOF_INPUT_ACL_DST_HOST_MEMBER
};

int acl_dst_host_member(DNDS_ACL *acl, void (*cb)(DNDS_HOST *))
{
	journal_ftrace(__func__);

	DNDS_HOST *res;

	HK_MYSQL_QUERY(acl_dst_host_member, ACL_DST_HOST_MEMBER);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_DST_HOST_MEMBER];
	MYSQL_BIND out[END_DNDS_HOST];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_HOST));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_HOST));
	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_DST_HOST_MEMBER);
	in[INPUT_ACL_DST_HOST_MEMBER_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_ACL_DST_HOST_MEMBER_CONTEXT].is_unsigned = 1;
	in[INPUT_ACL_DST_HOST_MEMBER_CONTEXT].buffer = (char *)&(acl->context);

	in[INPUT_ACL_DST_HOST_MEMBER_ACL].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_DST_HOST_MEMBER_ACL].is_unsigned = 1;
	in[INPUT_ACL_DST_HOST_MEMBER_ACL].buffer = (char *)&(acl->id);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_HOST);
	hk_mysql_bind_host(out, res);

	stmt.query = acl_dst_host_member;
	stmt.q_len = sizeof(ACL_DST_HOST_MEMBER);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * acl_dst_host_not_member
 */
#define ACL_DST_HOST_NOT_MEMBER \
\
	"SELECT " HOST_COLUMN " FROM " HOST_TABLE \
	" WHERE `context` = ? AND `id` = NOT IN" \
	" (SELECT `host` FROM " ACL_DST_MAP_TABLE " WHERE `acl` = ?)"

enum {
	INPUT_ACL_DST_HOST_NOT_MEMBER_CONTEXT = 0,
	INPUT_ACL_DST_HOST_NOT_MEMBER_ACL,
	SIZEOF_INPUT_ACL_DST_HOST_NOT_MEMBER
};

int acl_dst_host_not_member(DNDS_ACL *acl, void (*cb)(DNDS_HOST *))
{
	journal_ftrace(__func__);

	DNDS_HOST *res;

	HK_MYSQL_QUERY(acl_dst_host_not_member, ACL_DST_HOST_NOT_MEMBER);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_DST_HOST_NOT_MEMBER];
	MYSQL_BIND out[END_DNDS_HOST];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_HOST));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_HOST));
	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_DST_HOST_NOT_MEMBER);
	in[INPUT_ACL_DST_HOST_NOT_MEMBER_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_ACL_DST_HOST_NOT_MEMBER_CONTEXT].is_unsigned = 1;
	in[INPUT_ACL_DST_HOST_NOT_MEMBER_CONTEXT].buffer = (char *)&(acl->context);

	in[INPUT_ACL_DST_HOST_NOT_MEMBER_ACL].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_DST_HOST_NOT_MEMBER_ACL].is_unsigned = 1;
	in[INPUT_ACL_DST_HOST_NOT_MEMBER_ACL].buffer = (char *)&(acl->id);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_HOST);
	hk_mysql_bind_host(out, res);

	stmt.query = acl_dst_host_not_member;
	stmt.q_len = sizeof(ACL_DST_HOST_NOT_MEMBER);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * acl_dst_host_map
 */
#define ACL_DST_HOST_MAP \
\
	"INSERT INTO " ACL_DST_MAP_TABLE \
	" (`acl`, `host`) VALUES (?, ?)"

enum {
	INPUT_ACL_DST_HOST_MAP_ACL = 0,
	INPUT_ACL_DST_HOST_MAP_HOST,
	SIZEOF_INPUT_ACL_DST_HOST_MAP
};

int acl_dst_host_map(DNDS_ACL *acl, DNDS_HOST *host)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(acl_dst_host_map, ACL_DST_HOST_MAP);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_DST_HOST_MAP];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_DST_HOST_MAP);
	in[INPUT_ACL_DST_HOST_MAP_ACL].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_DST_HOST_MAP_ACL].is_unsigned = 1;
	in[INPUT_ACL_DST_HOST_MAP_ACL].buffer = (char *)&(acl->id);

	in[INPUT_ACL_DST_HOST_MAP_HOST].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_DST_HOST_MAP_HOST].is_unsigned = 1;
	in[INPUT_ACL_DST_HOST_MAP_HOST].buffer = (char *)&(host->id);

	stmt.query = acl_dst_host_map;
	stmt.q_len = sizeof(ACL_DST_HOST_MAP);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * acl_dst_host_unmap
 */
#define ACL_DST_HOST_UNMAP \
\
	"DELETE FROM " ACL_DST_MAP_TABLE \
	" WHERE `acl` = ? AND `host` = ?"

enum {
	INPUT_ACL_DST_HOST_UNMAP_ACL = 0,
	INPUT_ACL_DST_HOST_UNMAP_HOST,
	SIZEOF_INPUT_ACL_DST_HOST_UNMAP
};

int acl_dst_host_unmap(DNDS_ACL *acl, DNDS_HOST *host)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(acl_dst_host_unmap, ACL_DST_HOST_UNMAP);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_DST_HOST_UNMAP];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_DST_HOST_UNMAP);
	in[INPUT_ACL_DST_HOST_UNMAP_ACL].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_DST_HOST_UNMAP_ACL].is_unsigned = 1;
	in[INPUT_ACL_DST_HOST_UNMAP_ACL].buffer = (char *)&(acl->id);

	in[INPUT_ACL_DST_HOST_UNMAP_HOST].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_DST_HOST_UNMAP_HOST].is_unsigned = 1;
	in[INPUT_ACL_DST_HOST_UNMAP_HOST].buffer = (char *)&(host->id);

	stmt.query = acl_dst_host_unmap;
	stmt.q_len = sizeof(ACL_DST_HOST_UNMAP);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * acl_group_get
 */
#define ACL_GROUP_GET \
\
	"SELECT " ACL_GROUP_COLUMN " FROM " ACL_GROUP_TABLE \
	" WHERE `id` = ? AND `context` = ?"

enum {
	INPUT_ACL_GROUP_GET_ID = 0,
	INPUT_ACL_GROUP_GET_CONTEXT,

	SIZEOF_INPUT_ACL_GROUP_GET
};

DNDS_ACL_GROUP *acl_group_get(DNDS_ACL_GROUP *acl_group)
{
	journal_ftrace(__func__);

	DNDS_ACL_GROUP *res;

	HK_MYSQL_QUERY(acl_group_get, ACL_GROUP_GET);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_GROUP_GET];
	MYSQL_BIND out[END_DNDS_ACL_GROUP];
	struct stmt stmt;
	int ret = 0;

	res = malloc(sizeof(DNDS_ACL_GROUP));
	if (res == NULL)
		return NULL;

	memset(res, 0, sizeof(DNDS_ACL_GROUP));
	memset(&stmt, 0, sizeof(struct stmt));

	stmt.query = acl_group_get;
	stmt.q_len = sizeof(ACL_GROUP_GET);

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_GROUP_GET);
	in[INPUT_ACL_GROUP_GET_ID].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_GROUP_GET_ID].is_unsigned = 1;
	in[INPUT_ACL_GROUP_GET_ID].buffer = (char *)&(acl_group->id);

	in[INPUT_ACL_GROUP_GET_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_ACL_GROUP_GET_CONTEXT].is_unsigned = 1;
	in[INPUT_ACL_GROUP_GET_CONTEXT].buffer = (char *)&(acl_group->context);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_ACL_GROUP);
	hk_mysql_bind_acl_group(out, res);

	stmt.stmt = mysql_stmt_init(&mysql);
	if (hk_mysql_execute(&stmt)
	    || (ret = mysql_stmt_fetch(stmt.stmt))) {
		if (ret)
			journal_notice("%s%i :: %s:%i\n",
			    "mysql_stmt_fetch returned ",
			    ret, __FILE__, __LINE__);
		mysql_stmt_close(stmt.stmt);
		free(res);
		return NULL;
	}

	mysql_stmt_close(stmt.stmt);
	return res;
}

/*
 * acl_group_list
 */
#define ACL_GROUP_LIST \
\
	"SELECT " ACL_GROUP_COLUMN " FROM " ACL_GROUP_TABLE \
	" WHERE `context` = ?"

enum {
	INPUT_ACL_GROUP_LIST_CONTEXT = 0,
	SIZEOF_INPUT_ACL_GROUP_LIST
};

int acl_group_list(DNDS_ACL_GROUP *acl_group, void (*cb)(DNDS_ACL_GROUP *))
{
	journal_ftrace(__func__);

	DNDS_ACL_GROUP *res;

	HK_MYSQL_QUERY(acl_group_list, ACL_GROUP_LIST);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_GROUP_LIST];
	MYSQL_BIND out[END_DNDS_ACL_GROUP];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_ACL_GROUP));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_ACL_GROUP));
	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_GROUP_LIST);
	in[INPUT_ACL_GROUP_LIST_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_ACL_GROUP_LIST_CONTEXT].is_unsigned = 1;
	in[INPUT_ACL_GROUP_LIST_CONTEXT].buffer = (char *)&(acl_group->context);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_ACL_GROUP);
	hk_mysql_bind_acl_group(out, res);

	stmt.query = acl_group_list;
	stmt.q_len = sizeof(ACL_GROUP_LIST);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * acl_group_new
 */
#define ACL_GROUP_NEW \
\
	"INSERT INTO " ACL_GROUP_TABLE " ( " ACL_GROUP_COLUMN \
	" ) VALUES ( " ACL_GROUP_VALUE " )"

int acl_group_new(DNDS_ACL_GROUP *acl_group)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(acl_group_new, ACL_GROUP_NEW);
	MYSQL_BIND in[END_DNDS_ACL_GROUP];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, END_DNDS_ACL_GROUP);
	hk_mysql_bind_acl_group(in, acl_group);

	stmt.query = acl_group_new;
	stmt.q_len = sizeof(ACL_GROUP_NEW);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * acl_group_edit
 */
int acl_group_edit(DNDS_ACL_GROUP *acl_group)
{
	/* XXX */
}

/*
 * acl_group_clear
 */
int acl_group_clear(DNDS_ACL_GROUP *acl_group)
{
	/* XXX */
}

/*
 * acl_group_delete
 */
#define ACL_GROUP_DELETE \
\
	"DELETE FROM " ACL_GROUP_TABLE \
	" WHERE `id` = ? AND `context` = ?"

enum {
	INPUT_ACL_GROUP_DELETE_ID = 0,
	INPUT_ACL_GROUP_DELETE_CONTEXT,
	SIZEOF_INPUT_ACL_GROUP_DELETE
};

int acl_group_delete(DNDS_ACL_GROUP *acl_group)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(acl_group_delete, ACL_GROUP_DELETE);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_GROUP_DELETE];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_GROUP_DELETE);
	in[INPUT_ACL_GROUP_DELETE_ID].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_GROUP_DELETE_ID].is_unsigned = 1;
	in[INPUT_ACL_GROUP_DELETE_ID].buffer = (char *)&(acl_group->id);

	in[INPUT_ACL_GROUP_DELETE_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_ACL_GROUP_DELETE_CONTEXT].is_unsigned = 1;
	in[INPUT_ACL_GROUP_DELETE_CONTEXT].buffer = (char *)&(acl_group->context);

	stmt.query = acl_group_delete;
	stmt.q_len = sizeof(ACL_GROUP_DELETE);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * acl_group_host_member
 */
#define ACL_GROUP_HOST_MEMBER \
\
	"SELECT " HOST_COLUMN " FROM " HOST_TABLE " WHERE `context` = ?" \
	" AND `id` = ANY (SELECT `host` FROM " ACL_GROUP_MAP_TABLE \
	" WHERE `acl_group` = ?)"

enum {
	INPUT_ACL_GROUP_HOST_MEMBER_CONTEXT = 0,
	INPUT_ACL_GROUP_HOST_MEMBER_GROUP,
	SIZEOF_INPUT_ACL_GROUP_HOST_MEMBER
};

int acl_group_host_member(DNDS_ACL_GROUP *acl_group, void (*cb)(DNDS_HOST *))
{
	journal_ftrace(__func__);

	DNDS_HOST *res;

	HK_MYSQL_QUERY(acl_group_host_member, ACL_GROUP_HOST_MEMBER);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_GROUP_HOST_MEMBER];
	MYSQL_BIND out[END_DNDS_HOST];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_HOST));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_HOST));
	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_GROUP_HOST_MEMBER);
	in[INPUT_ACL_GROUP_HOST_MEMBER_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_ACL_GROUP_HOST_MEMBER_CONTEXT].is_unsigned = 1;
	in[INPUT_ACL_GROUP_HOST_MEMBER_CONTEXT].buffer = (char *)&(acl_group->context);

	in[INPUT_ACL_GROUP_HOST_MEMBER_GROUP].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_GROUP_HOST_MEMBER_GROUP].is_unsigned = 1;
	in[INPUT_ACL_GROUP_HOST_MEMBER_GROUP].buffer = (char *)&(acl_group->id);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_HOST);
	hk_mysql_bind_host(out, res);

	stmt.query = acl_group_host_member;
	stmt.q_len = sizeof(ACL_GROUP_HOST_MEMBER);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * acl_group_host_not_member
 */
#define ACL_GROUP_HOST_NOT_MEMBER \
\
	"SELECT " HOST_COLUMN " FROM " HOST_TABLE " WHERE `context` = ?" \
	" AND `id` NOT IN (SELECT `host` FROM " ACL_GROUP_MAP_TABLE \
	" WHERE `acl_group` = ?)"

enum {
	INPUT_ACL_GROUP_HOST_NOT_MEMBER_CONTEXT = 0,
	INPUT_ACL_GROUP_HOST_NOT_MEMBER_GROUP,
	SIZEOF_INPUT_ACL_GROUP_HOST_NOT_MEMBER
};

int acl_group_host_not_member(DNDS_ACL_GROUP *acl_group, void (*cb)(DNDS_HOST *))
{
	journal_ftrace(__func__);

	DNDS_HOST *res;

	HK_MYSQL_QUERY(acl_group_host_not_member, ACL_GROUP_HOST_NOT_MEMBER);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_GROUP_HOST_NOT_MEMBER];
	MYSQL_BIND out[END_DNDS_HOST];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_HOST));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_HOST));
	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_GROUP_HOST_NOT_MEMBER);
	in[INPUT_ACL_GROUP_HOST_NOT_MEMBER_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_ACL_GROUP_HOST_NOT_MEMBER_CONTEXT].is_unsigned = 1;
	in[INPUT_ACL_GROUP_HOST_NOT_MEMBER_CONTEXT].buffer = (char *)&(acl_group->context);

	in[INPUT_ACL_GROUP_HOST_NOT_MEMBER_GROUP].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_GROUP_HOST_NOT_MEMBER_GROUP].is_unsigned = 1;
	in[INPUT_ACL_GROUP_HOST_NOT_MEMBER_GROUP].buffer = (char *)&(acl_group->id);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_HOST);
	hk_mysql_bind_host(out, res);

	stmt.query = acl_group_host_not_member;
	stmt.q_len = sizeof(ACL_GROUP_HOST_NOT_MEMBER);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * acl_group_host_map
 */
#define ACL_GROUP_HOST_MAP \
\
	"INSERT INTO " ACL_GROUP_MAP_TABLE \
	" (`acl_group`, `host`) VALUES (?, ?)"

enum {
	INPUT_ACL_GROUP_HOST_MAP_GROUP = 0,
	INPUT_ACL_GROUP_HOST_MAP_HOST,
	SIZEOF_INPUT_ACL_GROUP_HOST_MAP
};

int acl_group_host_map(DNDS_ACL_GROUP *acl_group, DNDS_HOST *host)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(acl_group_host_map, ACL_GROUP_HOST_MAP);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_GROUP_HOST_MAP];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_GROUP_HOST_MAP);
	in[INPUT_ACL_GROUP_HOST_MAP_GROUP].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_GROUP_HOST_MAP_GROUP].is_unsigned = 1;
	in[INPUT_ACL_GROUP_HOST_MAP_GROUP].buffer = (char *)&(acl_group->id);

	in[INPUT_ACL_GROUP_HOST_MAP_HOST].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_GROUP_HOST_MAP_HOST].is_unsigned = 1;
	in[INPUT_ACL_GROUP_HOST_MAP_HOST].buffer = (char *)&(host->id);

	stmt.query = acl_group_host_map;
	stmt.q_len = sizeof(ACL_GROUP_HOST_MAP);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * acl_group_host_unmap
 */
#define ACL_GROUP_HOST_UNMAP \
\
	"DELETE FROM " ACL_GROUP_MAP_TABLE \
	" WHERE `acl_group` = ? AND `host` = ?"

enum {
	INPUT_ACL_GROUP_HOST_UNMAP_GROUP = 0,
	INPUT_ACL_GROUP_HOST_UNMAP_HOST,
	SIZEOF_INPUT_ACL_GROUP_HOST_UNMAP
};

int acl_group_host_unmap(DNDS_ACL_GROUP *acl_group, DNDS_HOST *host)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(acl_group_host_unmap, ACL_GROUP_HOST_UNMAP);
	MYSQL_BIND in[SIZEOF_INPUT_ACL_GROUP_HOST_UNMAP];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ACL_GROUP_HOST_UNMAP);
	in[INPUT_ACL_GROUP_HOST_UNMAP_GROUP].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_GROUP_HOST_UNMAP_GROUP].is_unsigned = 1;
	in[INPUT_ACL_GROUP_HOST_UNMAP_GROUP].buffer = (char *)&(acl_group->id);

	in[INPUT_ACL_GROUP_HOST_UNMAP_HOST].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ACL_GROUP_HOST_UNMAP_HOST].is_unsigned = 1;
	in[INPUT_ACL_GROUP_HOST_UNMAP_HOST].buffer = (char *)&(host->id);

	stmt.query = acl_group_host_unmap;
	stmt.q_len = sizeof(ACL_GROUP_HOST_UNMAP);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * addr_pool_get
 */
#define ADDR_POOL_GET \
\
	"SELECT " ADDR_POOL_COLUMN " FROM " ADDR_POOL_TABLE \
	" WHERE `id` = ?"

enum {
	INPUT_ADDR_POOL_GET_ID = 0,
	SIZEOF_INPUT_ADDR_POOL_GET
};

DNDS_ADDR_POOL *addr_pool_get(DNDS_ADDR_POOL *addr_pool)
{
	journal_ftrace(__func__);

	DNDS_ADDR_POOL *res;

	HK_MYSQL_QUERY(addr_pool_get, ADDR_POOL_GET);
	MYSQL_BIND in[SIZEOF_INPUT_ADDR_POOL_GET];
	MYSQL_BIND out[END_DNDS_ADDR_POOL];
	struct stmt stmt;
	int ret = 0;

	res = malloc(sizeof(DNDS_ADDR_POOL));
	if (res == NULL)
		return NULL;

	memset(res, 0, sizeof(DNDS_ADDR_POOL));
	memset(&stmt, 0, sizeof(struct stmt));

	stmt.query = addr_pool_get;
	stmt.q_len = sizeof(ADDR_POOL_GET);

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ADDR_POOL_GET);
	in[INPUT_ADDR_POOL_GET_ID].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ADDR_POOL_GET_ID].is_unsigned = 1;
	in[INPUT_ADDR_POOL_GET_ID].buffer = (char *)&(addr_pool->id);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_ADDR_POOL);
	hk_mysql_bind_addr_pool(out, res);

	stmt.stmt = mysql_stmt_init(&mysql);
	if (hk_mysql_execute(&stmt)
	    || (ret = mysql_stmt_fetch(stmt.stmt))) {
		if (ret)
			journal_notice("%s%i :: %s:%i\n",
			    "mysql_stmt_fetch returned ",
			    ret, __FILE__, __LINE__);
		mysql_stmt_close(stmt.stmt);
		free(res);
		return NULL;
	}

	mysql_stmt_close(stmt.stmt);
	return res;
}

/*
 * addr_pool_list
 */
#define ADDR_POOL_LIST \
\
	"SELECT " ADDR_POOL_COLUMN " FROM " ADDR_POOL_TABLE

int addr_pool_list(void (*cb)(DNDS_ADDR_POOL *))
{
	journal_ftrace(__func__);

	DNDS_ADDR_POOL *res;

	HK_MYSQL_QUERY(addr_pool_list, ADDR_POOL_LIST);
	MYSQL_BIND out[END_DNDS_ADDR_POOL];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_ADDR_POOL));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_ADDR_POOL));
	memset(&stmt, 0, sizeof(struct stmt));

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_ADDR_POOL);
	hk_mysql_bind_addr_pool(out, res);

	stmt.query = addr_pool_list;
	stmt.q_len = sizeof(ADDR_POOL_LIST);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * addr_pool_new
 */
#define ADDR_POOL_NEW \
\
	"INSERT INTO " ADDR_POOL_TABLE " ( " ADDR_POOL_COLUMN \
	" ) VALUES ( " ADDR_POOL_VALUE " )"

int addr_pool_new(DNDS_ADDR_POOL *addr_pool)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(addr_pool_new, ADDR_POOL_NEW);
	MYSQL_BIND in[END_DNDS_ADDR_POOL];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, END_DNDS_ADDR_POOL);
	hk_mysql_bind_addr_pool(in, addr_pool);

	stmt.query = addr_pool_new;
	stmt.q_len = sizeof(ADDR_POOL_NEW);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * addr_pool_edit
 */
int addr_pool_edit(DNDS_ADDR_POOL *addr_pool)
{
	/* XXX */
}

/*
 * addr_pool_clear
 */
int addr_pool_clear(DNDS_ADDR_POOL *addr_pool)
{
	/* XXX */
}

/*
 * addr_pool_delete
 */
#define ADDR_POOL_DELETE \
\
	"DELETE FROM " ADDR_POOL_TABLE \
	" WHERE `id` = ?"

enum {
	INPUT_ADDR_POOL_DELETE_ID = 0,
	SIZEOF_INPUT_ADDR_POOL_DELETE
};

int addr_pool_delete(DNDS_ADDR_POOL *addr_pool)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(addr_pool_delete, ADDR_POOL_DELETE);
	MYSQL_BIND in[SIZEOF_INPUT_ADDR_POOL_DELETE];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_ADDR_POOL_DELETE);
	in[INPUT_ADDR_POOL_DELETE_ID].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_ADDR_POOL_DELETE_ID].is_unsigned = 1;
	in[INPUT_ADDR_POOL_DELETE_ID].buffer = (char *)&(addr_pool->id);

	stmt.query = addr_pool_delete;
	stmt.q_len = sizeof(ADDR_POOL_DELETE);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * context_get
 */
#define CONTEXT_GET \
\
	"SELECT " CONTEXT_COLUMN " FROM " CONTEXT_TABLE \
	" WHERE `id` = ?"

enum {
	INPUT_CONTEXT_GET_ID = 0,
	SIZEOF_INPUT_CONTEXT_GET
};

DNDS_CONTEXT *context_get(DNDS_CONTEXT *context)
{
	journal_ftrace(__func__);

	DNDS_CONTEXT *res;

	HK_MYSQL_QUERY(context_get, CONTEXT_GET);
	MYSQL_BIND in[SIZEOF_INPUT_CONTEXT_GET];
	MYSQL_BIND out[END_DNDS_CONTEXT];
	struct stmt stmt;
	int ret = 0;

	res = malloc(sizeof(DNDS_CONTEXT));
	if (res == NULL)
		return NULL;

	memset(res, 0, sizeof(DNDS_CONTEXT));
	memset(&stmt, 0, sizeof(struct stmt));

	stmt.query = context_get;
	stmt.q_len = sizeof(CONTEXT_GET);

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_CONTEXT_GET);
	in[INPUT_CONTEXT_GET_ID].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_CONTEXT_GET_ID].is_unsigned = 1;
	in[INPUT_CONTEXT_GET_ID].buffer = (char *)&(context->id);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_CONTEXT);
	hk_mysql_bind_context(out, res);

	stmt.stmt = mysql_stmt_init(&mysql);
	if (hk_mysql_execute(&stmt)
	    || (ret = mysql_stmt_fetch(stmt.stmt))) {
		if (ret)
			journal_notice("%s%i :: %s:%i\n",
			    "mysql_stmt_fetch returned ",
			    ret, __FILE__, __LINE__);
		mysql_stmt_close(stmt.stmt);
		free(res);
		return NULL;
	}

	mysql_stmt_close(stmt.stmt);
	return res;
}

/*
 * context_list
 */
#define CONTEXT_LIST \
\
	"SELECT " CONTEXT_COLUMN " FROM " CONTEXT_TABLE

int context_list(void (*cb)(DNDS_CONTEXT *))
{
	journal_ftrace(__func__);

	DNDS_CONTEXT *res;

	HK_MYSQL_QUERY(context_list, CONTEXT_LIST);
	MYSQL_BIND out[END_DNDS_CONTEXT];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_CONTEXT));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_CONTEXT));
	memset(&stmt, 0, sizeof(struct stmt));

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_CONTEXT);
	hk_mysql_bind_context(out, res);

	stmt.query = context_list;
	stmt.q_len = sizeof(CONTEXT_LIST);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * context_new
 */
#define CONTEXT_NEW \
\
	"INSERT INTO " CONTEXT_TABLE " ( " CONTEXT_COLUMN \
	" ) VALUES ( " CONTEXT_VALUE " )"

int context_new(DNDS_CONTEXT *context)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(context_new, CONTEXT_NEW);
	MYSQL_BIND in[END_DNDS_CONTEXT];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, END_DNDS_CONTEXT);
	hk_mysql_bind_context(in, context);

	stmt.query = context_new;
	stmt.q_len = sizeof(CONTEXT_NEW);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * context_edit
 */
int context_edit(DNDS_CONTEXT *context)
{
	/* XXX */
}

/*
 * context_clear
 */
int context_clear(DNDS_CONTEXT *context)
{
	/* XXX */
}

/*
 * context_delete
 */
#define CONTEXT_DELETE \
\
	"DELETE FROM " CONTEXT_TABLE \
	" WHERE `id` = ?"

enum {
	INPUT_CONTEXT_DELETE_ID = 0,
	SIZEOF_INPUT_CONTEXT_DELETE
};

int context_delete(DNDS_CONTEXT *context)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(context_delete, CONTEXT_DELETE);
	MYSQL_BIND in[SIZEOF_INPUT_CONTEXT_DELETE];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_CONTEXT_DELETE);
	in[INPUT_CONTEXT_DELETE_ID].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_CONTEXT_DELETE_ID].is_unsigned = 1;
	in[INPUT_CONTEXT_DELETE_ID].buffer = (char *)&(context->id);

	stmt.query = context_delete;
	stmt.q_len = sizeof(CONTEXT_DELETE);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * host_get
 */
#define HOST_GET_BY_ID \
\
	"SELECT " HOST_COLUMN " FROM " HOST_TABLE \
	" WHERE `id` = ? AND `context` = ?"

enum {
	INPUT_HOST_GET_BY_ID = 0,
	INPUT_HOST_GET_BY_ID_CONTEXT,
	SIZEOF_INPUT_HOST_GET_BY_ID
};

#define HOST_GET_BY_HADDR \
\
	"SELECT " HOST_COLUMN " FROM " HOST_TABLE \
	" WHERE `haddr` LIKE ? AND `context` = ?"

enum {
	INPUT_HOST_GET_BY_HADDR = 0,
	INPUT_HOST_GET_BY_HADDR_CONTEXT,
	SIZEOF_INPUT_HOST_GET_BY_HADDR
};

DNDS_HOST *host_get(DNDS_HOST *host)
{
	journal_ftrace(__func__);

	DNDS_HOST *res;

	HK_MYSQL_QUERY(host_get_by_id, HOST_GET_BY_ID);
	HK_MYSQL_QUERY(host_get_by_haddr, HOST_GET_BY_HADDR);

	union {
		MYSQL_BIND id[SIZEOF_INPUT_HOST_GET_BY_ID];
		MYSQL_BIND haddr[SIZEOF_INPUT_HOST_GET_BY_HADDR];
	} in;

	MYSQL_BIND out[END_DNDS_HOST];
	struct stmt stmt;
	int ret = 0;

	res = malloc(sizeof(DNDS_HOST));
	if (res == NULL)
		return NULL;

	memset(res, 0, sizeof(DNDS_HOST));
	memset(&stmt, 0, sizeof(struct stmt));

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_HOST);
	hk_mysql_bind_host(out, res);

	/* input */
	if (host->id) {
		init_input_buffer(&stmt, in.id,
		    SIZEOF_INPUT_HOST_GET_BY_ID);
		stmt.query = host_get_by_id;
		stmt.q_len = sizeof(HOST_GET_BY_ID);

		in.id[INPUT_HOST_GET_BY_ID].buffer_type = MYSQL_TYPE_LONG;
		in.id[INPUT_HOST_GET_BY_ID].is_unsigned = 1;
		in.id[INPUT_HOST_GET_BY_ID].buffer = (char *)&(host->id);

		in.id[INPUT_HOST_GET_BY_ID_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
		in.id[INPUT_HOST_GET_BY_ID_CONTEXT].is_unsigned = 1;
		in.id[INPUT_HOST_GET_BY_ID_CONTEXT].buffer = (char *)&(host->context);

	} else if (host->haddr) {
		init_input_buffer(&stmt, in.haddr,
		    SIZEOF_INPUT_HOST_GET_BY_HADDR);
		stmt.query = host_get_by_haddr;
		stmt.q_len = sizeof(HOST_GET_BY_HADDR);

		in.haddr[INPUT_HOST_GET_BY_HADDR].buffer_type = MYSQL_TYPE_STRING;
		in.haddr[INPUT_HOST_GET_BY_HADDR].buffer_length = strlen((char *)host->haddr);
		in.haddr[INPUT_HOST_GET_BY_HADDR].buffer = (char *)&(host->haddr);

		in.haddr[INPUT_HOST_GET_BY_HADDR_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
		in.haddr[INPUT_HOST_GET_BY_HADDR_CONTEXT].is_unsigned = 1;
		in.haddr[INPUT_HOST_GET_BY_HADDR_CONTEXT].buffer = (char *)&(host->context);
	} else {
		journal_notice("mysql]> no such input :: %s:%i\n", __FILE__, __LINE__);
		free(res);
		return NULL;
	}

	stmt.stmt = mysql_stmt_init(&mysql);
	if (hk_mysql_execute(&stmt)
	    || (ret = mysql_stmt_fetch(stmt.stmt))) {
		if (ret)
			journal_notice("%s%i :: %s:%i\n",
			    "mysql_stmt_fetch returned ",
			    ret, __FILE__, __LINE__);
		mysql_stmt_close(stmt.stmt);
		free(res);
		return NULL;
	}

	mysql_stmt_close(stmt.stmt);
	return res;
}

/*
 * host_list
 */
#define HOST_LIST \
\
	"SELECT " HOST_COLUMN " FROM " HOST_TABLE \
	" WHERE `context` = ?"

enum {
	INPUT_HOST_LIST_CONTEXT = 0,
	SIZEOF_INPUT_HOST_LIST
};

int host_list(DNDS_HOST *host, void (*cb)(DNDS_HOST *))
{
	journal_ftrace(__func__);

	DNDS_HOST *res;

	HK_MYSQL_QUERY(host_list, HOST_LIST);
	MYSQL_BIND in[SIZEOF_INPUT_HOST_LIST];
	MYSQL_BIND out[END_DNDS_HOST];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_HOST));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_HOST));
	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_HOST_LIST);
	in[INPUT_HOST_LIST_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_HOST_LIST_CONTEXT].is_unsigned = 1;
	in[INPUT_HOST_LIST_CONTEXT].buffer = (char *)&(host->context);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_HOST);
	hk_mysql_bind_host(out, res);

	stmt.query = host_list;
	stmt.q_len = sizeof(HOST_LIST);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * host_new
 */
#define HOST_NEW \
\
	"INSERT INTO " HOST_TABLE " ( " HOST_COLUMN \
	" ) VALUES ( " HOST_VALUE " )"

int host_new(DNDS_HOST *host)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(host_new, HOST_NEW);
	MYSQL_BIND in[END_DNDS_HOST];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, END_DNDS_HOST);
	hk_mysql_bind_host(in, host);

	stmt.query = host_new;
	stmt.q_len = sizeof(HOST_NEW);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * host_edit
 */
int host_edit(DNDS_HOST *host)
{
	/* XXX */
}

/*
 * host_clear
 */
int host_clear(DNDS_HOST *host)
{
	/* XXX */
}

/*
 * host_delete
 */
#define HOST_DELETE \
\
	"DELETE FROM " HOST_TABLE \
	" WHERE `id` = ? AND `context` = ?"

enum {
	INPUT_HOST_DELETE_ID = 0,
	INPUT_HOST_DELETE_CONTEXT,
	SIZEOF_INPUT_HOST_DELETE
};

int host_delete(DNDS_HOST *host)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(host_delete, HOST_DELETE);
	MYSQL_BIND in[SIZEOF_INPUT_HOST_DELETE];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_HOST_DELETE);
	in[INPUT_HOST_DELETE_ID].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_HOST_DELETE_ID].is_unsigned = 1;
	in[INPUT_HOST_DELETE_ID].buffer = (char *)&(host->id);

	in[INPUT_HOST_DELETE_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_HOST_DELETE_CONTEXT].is_unsigned = 1;
	in[INPUT_HOST_DELETE_CONTEXT].buffer = (char *)&(host->context);

	stmt.query = host_delete;
	stmt.q_len = sizeof(HOST_DELETE);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * node_get
 */
#define NODE_GET_BY_ID \
\
	"SELECT " NODE_COLUMN " FROM " NODE_TABLE " WHERE `id` = ?"

#define NODE_GET_BY_NAME \
\
	"SELECT " NODE_COLUMN " FROM " NODE_TABLE " WHERE `name` LIKE ?"

DNDS_NODE *node_get(DNDS_NODE *node)
{
	journal_ftrace(__func__);

	DNDS_NODE *res;

	HK_MYSQL_QUERY(node_get_by_id, NODE_GET_BY_ID);
	HK_MYSQL_QUERY(node_get_by_name, NODE_GET_BY_NAME);

	MYSQL_BIND in, out[END_DNDS_NODE];
	struct stmt stmt;
	int ret = 0;

	res = malloc(sizeof(DNDS_NODE));
	if (res == NULL)
		return NULL;

	memset(res, 0, sizeof(DNDS_NODE));
	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, &in, 1);
	if (node->id) {
		stmt.query = node_get_by_id;
		stmt.q_len = sizeof(NODE_GET_BY_ID);
		in.buffer_type = MYSQL_TYPE_LONG;
		in.is_unsigned = 1;
		in.buffer = (char *)&(node->id);
	} else if (node->name) {
		stmt.query = node_get_by_name;
		stmt.q_len = sizeof(NODE_GET_BY_NAME);
		in.buffer_type = MYSQL_TYPE_STRING;
		in.buffer_length = strlen(node->name);
		in.buffer = node->name;
	} else {
		journal_notice("mysql]> no such input :: %s:%i\n", __FILE__, __LINE__);
		free(res);
		return NULL;
	}

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_NODE);
	hk_mysql_bind_node(out, res);

	stmt.stmt = mysql_stmt_init(&mysql);

	if (hk_mysql_execute(&stmt)
	    || (ret = mysql_stmt_fetch(stmt.stmt))) {
		if (ret)
			journal_notice("%s%i :: %s:%i\n",
			    "mysql_stmt_fetch returned ",
			    ret, __FILE__, __LINE__);
		mysql_stmt_close(stmt.stmt);
		free(res);
		return NULL;
	}

	mysql_stmt_close(stmt.stmt);

	return res;
}

/*
 * node_list
 */
#define NODE_LIST \
\
	"SELECT " NODE_COLUMN " FROM " NODE_TABLE

int node_list(void (*cb)(DNDS_NODE *))
{
	journal_ftrace(__func__);

	DNDS_NODE *res;

	HK_MYSQL_QUERY(node_list, NODE_LIST);
	MYSQL_BIND out[END_DNDS_NODE];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_NODE));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_NODE));
	memset(&stmt, 0, sizeof(struct stmt));

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_NODE);
	hk_mysql_bind_node(out, res);

	stmt.query = node_list;
	stmt.q_len = sizeof(NODE_LIST);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * node_new
 */
#define NODE_NEW \
\
	"INSERT INTO " NODE_TABLE " (" NODE_COLUMN ") VALUES (" NODE_VALUE ")"

int node_new(DNDS_NODE *node)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(node_new, NODE_NEW);
	MYSQL_BIND in[END_DNDS_NODE];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, END_DNDS_NODE);
	hk_mysql_bind_node(in, node);

	stmt.query = node_new;
	stmt.q_len = sizeof(NODE_NEW);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * node_edit
 */
int node_edit(DNDS_NODE *node)
{
	/* XXX */
}

/*
 * node_clear
 */
int node_clear(DNDS_NODE *node)
{
	/* XXX */
}

/*
 * node_delete
 */
#define NODE_DELETE \
\
	"DELETE FROM " NODE_TABLE \
	" WHERE `id` = ?"

enum {
	INPUT_NODE_DELETE_ID = 0,
	SIZEOF_INPUT_NODE_DELETE
};

int node_delete(DNDS_NODE *node)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(node_delete, NODE_DELETE);
	MYSQL_BIND in[SIZEOF_INPUT_NODE_DELETE];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_NODE_DELETE);
	in[INPUT_NODE_DELETE_ID].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_NODE_DELETE_ID].is_unsigned = 1;
	in[INPUT_NODE_DELETE_ID].buffer = (char *)&(node->id);

	stmt.query = node_delete;
	stmt.q_len = sizeof(NODE_DELETE);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * peer_get
 */
#define PEER_GET \
\
	"SELECT " PEER_COLUMN " FROM " PEER_TABLE \
	" WHERE `id` = ?"

enum {
	INPUT_PEER_GET_ID = 0,
	SIZEOF_INPUT_PEER_GET
};

DNDS_PEER *peer_get(DNDS_PEER *peer)
{
	journal_ftrace(__func__);

	DNDS_PEER *res;

	HK_MYSQL_QUERY(peer_get, PEER_GET);
	MYSQL_BIND in[SIZEOF_INPUT_PEER_GET];
	MYSQL_BIND out[END_DNDS_PEER];
	struct stmt stmt;
	int ret = 0;

	res = malloc(sizeof(DNDS_PEER));
	if (res == NULL)
		return NULL;

	memset(res, 0, sizeof(DNDS_PEER));
	memset(&stmt, 0, sizeof(struct stmt));

	stmt.query = peer_get;
	stmt.q_len = sizeof(PEER_GET);

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_PEER_GET);
	in[INPUT_PEER_GET_ID].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_PEER_GET_ID].is_unsigned = 1;
	in[INPUT_PEER_GET_ID].buffer = (char *)&(peer->id);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_PEER);
	hk_mysql_bind_peer(out, res);

	stmt.stmt = mysql_stmt_init(&mysql);
	if (hk_mysql_execute(&stmt)
	    || (ret = mysql_stmt_fetch(stmt.stmt))) {
		if (ret)
			journal_notice("%s%i :: %s:%i\n",
			    "mysql_stmt_fetch returned ",
			    ret, __FILE__, __LINE__);
		mysql_stmt_close(stmt.stmt);
		free(res);
		return NULL;
	}

	mysql_stmt_close(stmt.stmt);
	return res;
}

/*
 * peer_list
 */
#define PEER_LIST \
\
	"SELECT " PEER_COLUMN " FROM " PEER_TABLE \
	" WHERE `context` = ?"

enum {
	INPUT_PEER_LIST_CONTEXT = 0,
	SIZEOF_INPUT_PEER_LIST
};

int peer_list(DNDS_PEER *peer, void (*cb)(DNDS_PEER *))
{
	journal_ftrace(__func__);

	DNDS_PEER *res;

	HK_MYSQL_QUERY(peer_list, PEER_LIST);
	MYSQL_BIND in[SIZEOF_INPUT_PEER_LIST];
	MYSQL_BIND out[END_DNDS_PEER];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_PEER));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_PEER));
	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_PEER_LIST);
	in[INPUT_PEER_LIST_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_PEER_LIST_CONTEXT].is_unsigned = 1;
	in[INPUT_PEER_LIST_CONTEXT].buffer = (char *)&(peer->context);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_PEER);
	hk_mysql_bind_peer(out, res);

	stmt.query = peer_list;
	stmt.q_len = sizeof(PEER_LIST);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * peer_new
 */
#define PEER_NEW \
\
	"INSERT INTO " PEER_TABLE " ( " PEER_COLUMN \
	" ) VALUES ( " PEER_VALUE " )"

int peer_new(DNDS_PEER *peer)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(peer_new, PEER_NEW);
	MYSQL_BIND in[END_DNDS_PEER];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, END_DNDS_PEER);
	hk_mysql_bind_peer(in, peer);

	stmt.query = peer_new;
	stmt.q_len = sizeof(PEER_NEW);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * peer_edit
 */
int peer_edit(DNDS_PEER *peer)
{
	/* XXX */
}

/*
 * peer_clear
 */
int peer_clear(DNDS_PEER *peer)
{
	/* XXX */
}

/*
 * peer_delete
 */
#define PEER_DELETE \
\
	"DELETE FROM " PEER_TABLE \
	" WHERE `id` = ? AND `context` = ?"

enum {
	INPUT_PEER_DELETE_ID = 0,
	INPUT_PEER_DELETE_CONTEXT,
	SIZEOF_INPUT_PEER_DELETE
};

int peer_delete(DNDS_PEER *peer)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(peer_delete, PEER_DELETE);
	MYSQL_BIND in[SIZEOF_INPUT_PEER_DELETE];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_PEER_DELETE);
	in[INPUT_PEER_DELETE_ID].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_PEER_DELETE_ID].is_unsigned = 1;
	in[INPUT_PEER_DELETE_ID].buffer = (char *)&(peer->id);

	in[INPUT_PEER_DELETE_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_PEER_DELETE_CONTEXT].is_unsigned = 1;
	in[INPUT_PEER_DELETE_CONTEXT].buffer = (char *)&(peer->context);

	stmt.query = peer_delete;
	stmt.q_len = sizeof(PEER_DELETE);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * peer_user_member
 */
#define PEER_USER_MEMBER \
\
	"SELECT " USER_COLUMN " FROM " USER_TABLE " WHERE `context` = ?" \
	" AND `id` = ANY (SELECT `user` FROM " PEER_USER_MAP_TABLE \
	" WHERE `peer` = ?)"

enum {
	INPUT_PEER_USER_MEMBER_CONTEXT = 0,
	INPUT_PEER_USER_MEMBER_PEER,
	SIZEOF_INPUT_PEER_USER_MEMBER
};

int peer_user_member(DNDS_PEER *peer, void (*cb)(DNDS_USER *))
{
	journal_ftrace(__func__);

	DNDS_USER *res;

	HK_MYSQL_QUERY(peer_user_member, PEER_USER_MEMBER);
	MYSQL_BIND in[SIZEOF_INPUT_PEER_USER_MEMBER];
	MYSQL_BIND out[END_DNDS_USER];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_USER));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_USER));
	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_PEER_USER_MEMBER);
	in[INPUT_PEER_USER_MEMBER_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_PEER_USER_MEMBER_CONTEXT].is_unsigned = 1;
	in[INPUT_PEER_USER_MEMBER_CONTEXT].buffer = (char *)&(peer->context);

	in[INPUT_PEER_USER_MEMBER_PEER].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_PEER_USER_MEMBER_PEER].is_unsigned = 1;
	in[INPUT_PEER_USER_MEMBER_PEER].buffer = (char *)&(peer->id);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_USER);
	hk_mysql_bind_user(out, res);

	stmt.query = peer_user_member;
	stmt.q_len = sizeof(PEER_USER_MEMBER);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * peer_user_not_member
 */
#define PEER_USER_NOT_MEMBER \
\
	"SELECT " USER_COLUMN " FROM " USER_TABLE " WHERE `context` = ?" \
	" AND `id` NOT IN (SELECT `user` FROM " PEER_USER_MAP_TABLE \
	" WHERE `peer` = ?)"

enum {
	INPUT_PEER_USER_NOT_MEMBER_CONTEXT = 0,
	INPUT_PEER_USER_NOT_MEMBER_PEER,
	SIZEOF_INPUT_PEER_USER_NOT_MEMBER
};

int peer_user_not_member(DNDS_PEER *peer, void (*cb)(DNDS_USER *))
{
	journal_ftrace(__func__);

	DNDS_USER *res;

	HK_MYSQL_QUERY(peer_user_not_member, PEER_USER_NOT_MEMBER);
	MYSQL_BIND in[SIZEOF_INPUT_PEER_USER_NOT_MEMBER];
	MYSQL_BIND out[END_DNDS_USER];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_USER));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_USER));
	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_PEER_USER_NOT_MEMBER);
	in[INPUT_PEER_USER_NOT_MEMBER_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_PEER_USER_NOT_MEMBER_CONTEXT].is_unsigned = 1;
	in[INPUT_PEER_USER_NOT_MEMBER_CONTEXT].buffer = (char *)&(peer->context);

	in[INPUT_PEER_USER_NOT_MEMBER_PEER].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_PEER_USER_NOT_MEMBER_PEER].is_unsigned = 1;
	in[INPUT_PEER_USER_NOT_MEMBER_PEER].buffer = (char *)&(peer->id);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_USER);
	hk_mysql_bind_user(out, res);

	stmt.query = peer_user_not_member;
	stmt.q_len = sizeof(PEER_USER_NOT_MEMBER);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * peer_user_map
 */
#define PEER_USER_MAP \
\
	"INSERT INTO " PEER_USER_MAP_TABLE \
	" (`peer`, `user`) VALUES (?, ?)"

enum {
	INPUT_PEER_USER_MAP_PEER = 0,
	INPUT_PEER_USER_MAP_USER,
	SIZEOF_INPUT_PEER_USER_MAP
};

int peer_user_map(DNDS_PEER *peer, DNDS_USER *user)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(peer_user_map, PEER_USER_MAP);
	MYSQL_BIND in[SIZEOF_INPUT_PEER_USER_MAP];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_PEER_USER_MAP);
	in[INPUT_PEER_USER_MAP_PEER].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_PEER_USER_MAP_PEER].is_unsigned = 1;
	in[INPUT_PEER_USER_MAP_PEER].buffer = (char *)&(peer->id);

	in[INPUT_PEER_USER_MAP_USER].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_PEER_USER_MAP_USER].is_unsigned = 1;
	in[INPUT_PEER_USER_MAP_USER].buffer = (char *)&(user->id);

	stmt.query = peer_user_map;
	stmt.q_len = sizeof(PEER_USER_MAP);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * peer_user_unmap
 */
#define PEER_USER_UNMAP \
\
	"DELETE FROM " PEER_USER_MAP_TABLE \
	" WHERE `peer` = ? AND `user` = ?"

enum {
	INPUT_PEER_USER_UNMAP_PEER = 0,
	INPUT_PEER_USER_UNMAP_USER,
	SIZEOF_INPUT_PEER_USER_UNMAP
};

int peer_user_unmap(DNDS_PEER *peer, DNDS_USER *user)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(peer_user_unmap, PEER_USER_UNMAP);
	MYSQL_BIND in[SIZEOF_INPUT_PEER_USER_UNMAP];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_PEER_USER_UNMAP);
	in[INPUT_PEER_USER_UNMAP_PEER].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_PEER_USER_UNMAP_PEER].is_unsigned = 1;
	in[INPUT_PEER_USER_UNMAP_PEER].buffer = (char *)&(peer->id);

	in[INPUT_PEER_USER_UNMAP_USER].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_PEER_USER_UNMAP_USER].is_unsigned = 1;
	in[INPUT_PEER_USER_UNMAP_USER].buffer = (char *)&(user->id);

	stmt.query = peer_user_unmap;
	stmt.q_len = sizeof(PEER_USER_UNMAP);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * peer_host_member
 */
#define PEER_HOST_MEMBER \
\
	"SELECT " HOST_COLUMN " FROM " HOST_TABLE \
	" WHERE `context` = ? AND `peer` = ?" 

enum {
	INPUT_PEER_HOST_MEMBER_CONTEXT = 0,
	INPUT_PEER_HOST_MEMBER_PEER,
	SIZEOF_INPUT_PEER_HOST_MEMBER
};

int peer_host_member(DNDS_PEER *peer, void (*cb)(DNDS_HOST *))
{
	journal_ftrace(__func__);

	DNDS_HOST *res;

	HK_MYSQL_QUERY(peer_host_member, PEER_HOST_MEMBER);
	MYSQL_BIND in[SIZEOF_INPUT_PEER_HOST_MEMBER];
	MYSQL_BIND out[END_DNDS_HOST];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_HOST));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_HOST));
	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_PEER_HOST_MEMBER);
	in[INPUT_PEER_HOST_MEMBER_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_PEER_HOST_MEMBER_CONTEXT].is_unsigned = 1;
	in[INPUT_PEER_HOST_MEMBER_CONTEXT].buffer = (char *)&(peer->context);

	in[INPUT_PEER_HOST_MEMBER_PEER].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_PEER_HOST_MEMBER_PEER].is_unsigned = 1;
	in[INPUT_PEER_HOST_MEMBER_PEER].buffer = (char *)&(peer->id);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_HOST);
	hk_mysql_bind_host(out, res);

	stmt.query = peer_host_member;
	stmt.q_len = sizeof(PEER_HOST_MEMBER);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * peer_host_not_member
 */
#define PEER_HOST_NOT_MEMBER \
\
	"SELECT " HOST_COLUMN " FROM " HOST_TABLE \
	" WHERE `context` = ? AND `peer` <> ?" 

enum {
	INPUT_PEER_HOST_NOT_MEMBER_CONTEXT = 0,
	INPUT_PEER_HOST_NOT_MEMBER_PEER,
	SIZEOF_INPUT_PEER_HOST_NOT_MEMBER
};

int peer_host_not_member(DNDS_PEER *peer, void (*cb)(DNDS_HOST *))
{
	journal_ftrace(__func__);

	DNDS_HOST *res;

	HK_MYSQL_QUERY(peer_host_not_member, PEER_HOST_NOT_MEMBER);
	MYSQL_BIND in[SIZEOF_INPUT_PEER_HOST_NOT_MEMBER];
	MYSQL_BIND out[END_DNDS_HOST];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_HOST));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_HOST));
	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_PEER_HOST_NOT_MEMBER);
	in[INPUT_PEER_HOST_NOT_MEMBER_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_PEER_HOST_NOT_MEMBER_CONTEXT].is_unsigned = 1;
	in[INPUT_PEER_HOST_NOT_MEMBER_CONTEXT].buffer = (char *)&(peer->context);

	in[INPUT_PEER_HOST_NOT_MEMBER_PEER].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_PEER_HOST_NOT_MEMBER_PEER].is_unsigned = 1;
	in[INPUT_PEER_HOST_NOT_MEMBER_PEER].buffer = (char *)&(peer->id);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_HOST);
	hk_mysql_bind_host(out, res);

	stmt.query = peer_host_not_member;
	stmt.q_len = sizeof(PEER_HOST_NOT_MEMBER);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * peer_host_map
 */
#define PEER_HOST_MAP \
\
	"UPDATE " HOST_TABLE " SET `peer` = ?" \
	" WHERE `context` = ? AND `id` = ?"

enum {
	INPUT_PEER_HOST_MAP_PEER = 0,
	INPUT_PEER_HOST_MAP_CONTEXT,
	INPUT_PEER_HOST_MAP_ID,
	SIZEOF_INPUT_PEER_HOST_MAP
};

int peer_host_map(DNDS_PEER *peer, DNDS_HOST *host)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(peer_host_map, PEER_HOST_MAP);
	MYSQL_BIND in[SIZEOF_INPUT_PEER_HOST_MAP];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_PEER_HOST_MAP);
	in[INPUT_PEER_HOST_MAP_PEER].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_PEER_HOST_MAP_PEER].is_unsigned = 1;
	in[INPUT_PEER_HOST_MAP_PEER].buffer = (char *)&(peer->id);

	in[INPUT_PEER_HOST_MAP_CONTEXT].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_PEER_HOST_MAP_CONTEXT].is_unsigned = 1;
	in[INPUT_PEER_HOST_MAP_CONTEXT].buffer = (char *)&(peer->context);

	in[INPUT_PEER_HOST_MAP_ID].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_PEER_HOST_MAP_ID].is_unsigned = 1;
	in[INPUT_PEER_HOST_MAP_ID].buffer = (char *)&(host->id);

	stmt.query = peer_host_map;
	stmt.q_len = sizeof(PEER_HOST_MAP);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * peer_host_unmap
 */
int peer_host_unmap(DNDS_PEER *peer, DNDS_HOST *host)
{
	journal_notice("mysql]> %s not implemented :: %s:%i\n",
	    __func__, __FILE__, __LINE__);
	return -1;
}

/*
 * perm_get
 */
#define PERM_GET \
\
	"SELECT " PERM_COLUMN " FROM " PERM_TABLE " WHERE `id` = ?"

DNDS_PERM *perm_get(DNDS_PERM *perm)
{
	journal_ftrace(__func__);

	DNDS_PERM *res;

	HK_MYSQL_QUERY(perm_get, PERM_GET);
	MYSQL_BIND in;
	MYSQL_BIND out[END_DNDS_PERM];
	struct stmt stmt;
	int ret = 0;

	res = malloc(sizeof(DNDS_PERM));
	if (res == NULL)
		return NULL;

	memset(res, 0, sizeof(DNDS_PERM));
	memset(&stmt, 0, sizeof(struct stmt));

	stmt.query = perm_get;
	stmt.q_len = sizeof(PERM_GET);

	/* input */
	init_input_buffer(&stmt, &in, 1);
	in.buffer_type = MYSQL_TYPE_LONG;
	in.is_unsigned = 1;
	in.buffer = (char *)&(perm->id);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_PERM);
	hk_mysql_bind_perm(out, res);

	stmt.stmt = mysql_stmt_init(&mysql);
	if (hk_mysql_execute(&stmt)
	    || (ret = mysql_stmt_fetch(stmt.stmt))) {
		if (ret)
			journal_notice("%s%i :: %s:%i\n",
			    "mysql_stmt_fetch returned ",
			    ret, __FILE__, __LINE__);
		mysql_stmt_close(stmt.stmt);
		free(res);
		return NULL;
	}

	mysql_stmt_close(stmt.stmt);
	return res;
}

/*
 * perm_list
 */
#define PERM_LIST \
\
	"SELECT " PERM_COLUMN " FROM " PERM_TABLE

int perm_list(void (*cb)(DNDS_PERM *))
{
	journal_ftrace(__func__);

	DNDS_PERM *res;

	HK_MYSQL_QUERY(perm_list, PERM_LIST);
	MYSQL_BIND out[END_DNDS_PERM];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_PERM));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_PERM));
	memset(&stmt, 0, sizeof(struct stmt));

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_PERM);
	hk_mysql_bind_perm(out, res);

	stmt.query = perm_list;
	stmt.q_len = sizeof(PERM_LIST);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * perm_new
 */
#define PERM_NEW \
\
	"INSERT INTO " PERM_TABLE " (" PERM_COLUMN ") VALUES (" PERM_VALUE ")"

int perm_new(DNDS_PERM *perm)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(perm_new, PERM_NEW);
	MYSQL_BIND in[END_DNDS_PERM];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, END_DNDS_PERM);
	hk_mysql_bind_perm(in, perm);

	stmt.query = perm_new;
	stmt.q_len = sizeof(PERM_NEW);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * perm_edit
 */
int perm_edit(DNDS_PERM *perm)
{
	/* XXX */
}

/*
 * perm_clear
 */
int perm_clear(DNDS_PERM *perm)
{
	/* XXX */
}

/*
 * perm_delete
 */
#define PERM_DELETE \
\
	"DELETE FROM " PERM_TABLE \
	" WHERE `id` = ?"

enum {
	INPUT_PERM_DELETE_ID = 0,
	SIZEOF_INPUT_PERM_DELETE
};

int perm_delete(DNDS_PERM *perm)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(perm_delete, PERM_DELETE);
	MYSQL_BIND in[SIZEOF_INPUT_PERM_DELETE];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_PERM_DELETE);
	in[INPUT_PERM_DELETE_ID].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_PERM_DELETE_ID].is_unsigned = 1;
	in[INPUT_PERM_DELETE_ID].buffer = (char *)&(perm->id);

	stmt.query = perm_delete;
	stmt.q_len = sizeof(PERM_DELETE);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * user_get
 */
#define USER_GET \
\
	"SELECT " USER_COLUMN " FROM " USER_TABLE \
	" WHERE `id` = ?"

enum {
	INPUT_USER_GET_ID = 0,
	SIZEOF_INPUT_USER_GET
};

DNDS_USER *user_get(DNDS_USER *user)
{
	journal_ftrace(__func__);

	DNDS_USER *res;

	HK_MYSQL_QUERY(user_get, USER_GET);
	MYSQL_BIND in[SIZEOF_INPUT_USER_GET];
	MYSQL_BIND out[END_DNDS_USER];
	struct stmt stmt;
	int ret = 0;

	res = malloc(sizeof(DNDS_USER));
	if (res == NULL)
		return NULL;

	memset(res, 0, sizeof(DNDS_USER));
	memset(&stmt, 0, sizeof(struct stmt));

	stmt.query = user_get;
	stmt.q_len = sizeof(USER_GET);

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_USER_GET);
	in[INPUT_USER_GET_ID].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_USER_GET_ID].is_unsigned = 1;
	in[INPUT_USER_GET_ID].buffer = (char *)&(user->id);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_USER);
	hk_mysql_bind_user(out, res);

	stmt.stmt = mysql_stmt_init(&mysql);
	if (hk_mysql_execute(&stmt)
	    || (ret = mysql_stmt_fetch(stmt.stmt))) {
		if (ret)
			journal_notice("%s%i :: %s:%i\n",
			    "mysql_stmt_fetch returned ",
			    ret, __FILE__, __LINE__);
		mysql_stmt_close(stmt.stmt);
		free(res);
		return NULL;
	}

	mysql_stmt_close(stmt.stmt);
	return res;
}

/*
 * user_list
 */
#define USER_LIST \
\
	"SELECT " USER_COLUMN " FROM " USER_TABLE \
	" WHERE `context` = ?"

enum {
	INPUT_USER_LIST_CONTEXT = 0,
	SIZEOF_INPUT_USER_LIST
};

int user_list(DNDS_USER *user, void (*cb)(DNDS_USER *))
{
	journal_ftrace(__func__);

	DNDS_USER *res;

	HK_MYSQL_QUERY(user_list, USER_LIST);
	MYSQL_BIND in[SIZEOF_INPUT_USER_LIST];
	MYSQL_BIND out[END_DNDS_USER];
	struct stmt stmt;
	int ret;

	res = malloc(sizeof(DNDS_USER));
	if (res == NULL)
		return -1;

	memset(res, 0, sizeof(DNDS_USER));
	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_USER_LIST);
	in[INPUT_USER_LIST_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_USER_LIST_CONTEXT].is_unsigned = 1;
	in[INPUT_USER_LIST_CONTEXT].buffer = (char *)&(user->context);

	/* output */
	init_output_buffer(&stmt, out, END_DNDS_USER);
	hk_mysql_bind_user(out, res);

	stmt.query = user_list;
	stmt.q_len = sizeof(USER_LIST);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (!hk_mysql_execute(&stmt))
		while (!mysql_stmt_fetch(stmt.stmt))
			cb(res);

	mysql_stmt_close(stmt.stmt);
	free(res);
	return ret;
}

/*
 * user_new
 */
#define USER_NEW \
\
	"INSERT INTO " USER_TABLE " ( " USER_COLUMN \
	" ) VALUES ( " USER_VALUE " )"

int user_new(DNDS_USER *user)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(user_new, USER_NEW);
	MYSQL_BIND in[END_DNDS_USER];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, END_DNDS_USER);
	hk_mysql_bind_user(in, user);

	stmt.query = user_new;
	stmt.q_len = sizeof(USER_NEW);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * user_edit
 */
int user_edit(DNDS_USER *user)
{
	/* XXX */
}

/*
 * user_clear
 */
int user_clear(DNDS_USER *user)
{
	/* XXX */
}

/*
 * user_delete
 */
#define USER_DELETE \
\
	"DELETE FROM " USER_TABLE \
	" WHERE `id` = ? AND `context` = ?"

enum {
	INPUT_USER_DELETE_ID = 0,
	INPUT_USER_DELETE_CONTEXT,
	SIZEOF_INPUT_USER_DELETE
};

int user_delete(DNDS_USER *user)
{
	journal_ftrace(__func__);

	HK_MYSQL_QUERY(user_delete, USER_DELETE);
	MYSQL_BIND in[SIZEOF_INPUT_USER_DELETE];
	struct stmt stmt;

	memset(&stmt, 0, sizeof(struct stmt));

	/* input */
	init_input_buffer(&stmt, in, SIZEOF_INPUT_USER_DELETE);
	in[INPUT_USER_DELETE_ID].buffer_type = MYSQL_TYPE_LONG;
	in[INPUT_USER_DELETE_ID].is_unsigned = 1;
	in[INPUT_USER_DELETE_ID].buffer = (char *)&(user->id);

	in[INPUT_USER_DELETE_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;
	in[INPUT_USER_DELETE_CONTEXT].is_unsigned = 1;
	in[INPUT_USER_DELETE_CONTEXT].buffer = (char *)&(user->context);

	stmt.query = user_delete;
	stmt.q_len = sizeof(USER_DELETE);

	stmt.stmt = mysql_stmt_init(&mysql);
	
	if (hk_mysql_execute(&stmt)
	    || !mysql_stmt_affected_rows(stmt.stmt)) {
		mysql_stmt_close(stmt.stmt);
		return -1;
	}

	mysql_stmt_close(stmt.stmt);
	return 0;
}

/*
 * db_ping
 */
int db_ping()
{
	journal_ftrace(__func__);

	if (mysql_ping(&mysql)) {
	    journal_notice("mysql]> mysql_ping failed, %s :: %s:%i\n",
			    mysql_error(&mysql), __FILE__, __LINE__);
			return -1;
	}
	return 0;
}

int db_init(const char *host, const char *user, const char *passwd, const char *db)
{
	journal_ftrace(__func__);

	static const unsigned timeout = 10; /* should be taken from config */
	int err = 0;
	my_bool	reconnect = 1; /* should be taken from config */

	if (mysql_init(&mysql) == NULL) {
		journal_notice("mysql]> mysql_init() %s :: %s:%i\n",
		    mysql_error(&mysql), __FILE__, __LINE__);
		return -1;
	}

	err = mysql_options(&mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const char *)&timeout);
	if (err) {
		mysql_close(&mysql);
		journal_notice("mysql]> mysql_options() %s :: %s:%i\n",
		    mysql_error(&mysql), __FILE__, __LINE__);
		return -1;
	}

	err = mysql_options(&mysql, MYSQL_OPT_RECONNECT, (const char *)&reconnect);
	if (err) {
		mysql_close(&mysql);
		journal_notice("mysql]> mysql_options() %s :: %s:%i\n",
		    mysql_error(&mysql), __FILE__, __LINE__);
		return -1;
	}

	if (mysql_real_connect(&mysql, host, user, passwd, db, 0, NULL, 0) == NULL) {
		mysql_close(&mysql);
		 journal_notice("mysql]> %s :: %s:%i\n", mysql_error(&mysql), __FILE__, __LINE__);
		return -1;
	}

	return 0;
}

int db_fini()
{
	journal_ftrace(__func__);

	return 0;
}

int hookin()
{
	journal_ftrace(__func__);
	return HOOKLET_DBAL;
}
