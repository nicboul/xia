/*
 * Copyright 2008 Mind4Networks Technologies INC.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <mysql/mysql.h>
#include <mysql/errmsg.h>

#include "../../lib/hooklet.h"
#include "../../lib/journal.h"
#include "../../lib/utils.h"

#define MYSQL_BIND_SIZE sizeof(MYSQL_BIND)

#define STMT_SQL_QUERY(name, sql) char name[sizeof(sql)] = sql

MYSQL mysql;
MYSQL_STMT *mysql_stmt;

struct stmt_singleton {
	char	*stmt;		/* SQL statement */
	size_t	stmt_length;	/* Length of SQL statement */

	MYSQL_BIND *in_buf;	/* MySQL meta structure for input buffers */
	uint8_t in_buf_count;	/* Number of input buffers */
	MYSQL_BIND *out_buf;	/* MySQL meta structure for output buffers */
	uint8_t out_buf_count;	/* Number of output buffers */
};

void init_input_buffer(struct stmt_singleton *s,
			  MYSQL_BIND *buf, size_t buf_size)
{
	journal_strace("init_input_buffer");

	s->in_buf = buf;
	s->in_buf_count = buf_size / MYSQL_BIND_SIZE; /* FIXME This assumption is error prone */
	memset(buf, 0, buf_size);
}

void init_output_buffer(struct stmt_singleton *s,
			  MYSQL_BIND *buf, size_t buf_size)
{
	journal_strace("init_output_buffer");

	s->out_buf = buf;
	s->out_buf_count = buf_size / MYSQL_BIND_SIZE; /* FIXME This assumption is error prone */
	memset(buf, 0, buf_size);
}

int_vector *mysql_exec(struct stmt_singleton *);

/*
 * get_token_type method
 */
#define GET_TOKEN_TYPE \
\
        "SELECT `type` FROM `tokens` WHERE `id` = XIA_TOKEN(?)"

struct stmt_singleton *get_token_type_init_once()
{
	journal_strace("get_token_type_init_once");

	static struct stmt_singleton stmt;
	static MYSQL_BIND in_buf;
	static MYSQL_BIND out_buf;
	static STMT_SQL_QUERY(sql_stmt, GET_TOKEN_TYPE);
	
	if (stmt.stmt_length == sizeof(GET_TOKEN_TYPE))
		return &stmt;

	memset(&stmt, 0, sizeof(stmt));
	init_input_buffer(&stmt, &in_buf, sizeof(in_buf));
	init_output_buffer(&stmt, &out_buf, sizeof(out_buf));

	stmt.stmt = sql_stmt;
	stmt.stmt_length = sizeof(GET_TOKEN_TYPE);
	in_buf.buffer_type = MYSQL_TYPE_STRING;
	out_buf.buffer_type = MYSQL_TYPE_TINY;

	return &stmt;
}

int_vector *get_token_type(char *name)
{
	journal_strace("get_token_type");

	struct stmt_singleton *stmt;
	stmt = get_token_type_init_once();

	stmt->in_buf->buffer = name;
	stmt->in_buf->buffer_length = strlen(name);

	return mysql_exec(stmt);
}

#define GET_TOKEN_STATUS \
\
        "SELECT `status` FROM `tokens` WHERE `id` = XIA_TOKEN(?)"

struct stmt_singleton *get_token_status_init_once()
{
	journal_strace("get_token_status_init_once");

	static struct stmt_singleton stmt;
	static MYSQL_BIND in_buf;
	static MYSQL_BIND out_buf;
	static STMT_SQL_QUERY(sql_stmt, GET_TOKEN_STATUS);
	
	if (stmt.stmt_length == sizeof(GET_TOKEN_STATUS))
		return &stmt;

	memset(&stmt, 0, sizeof(stmt));
	init_input_buffer(&stmt, &in_buf, sizeof(in_buf));
	init_output_buffer(&stmt, &out_buf, sizeof(out_buf));

	stmt.stmt = sql_stmt;
	stmt.stmt_length = sizeof(GET_TOKEN_STATUS);
	in_buf.buffer_type = MYSQL_TYPE_STRING;
	out_buf.buffer_type = MYSQL_TYPE_TINY;

	return &stmt;
}

int_vector *get_token_status(char *name)
{
	journal_strace("get_token_status");

	struct stmt_singleton *stmt;
	stmt = get_token_status_init_once();

	stmt->in_buf->buffer = name;
	stmt->in_buf->buffer_length = strlen(name);

	return mysql_exec(stmt);
}

#define GET_RES_MEMBERSHIP \
\
        "SELECT `acl_set` FROM `acl_sets_res_map` WHERE `token` = XIA_TOKEN(?)"

struct stmt_singleton *get_res_membership_init_once()
{
	journal_strace("get_res_membership_init_once");

	static struct stmt_singleton stmt;
	static MYSQL_BIND in_buf;
	static MYSQL_BIND out_buf;
	static STMT_SQL_QUERY(sql_stmt, GET_RES_MEMBERSHIP);

	if (stmt.stmt_length == sizeof(GET_RES_MEMBERSHIP))
		return &stmt;

	memset(&stmt, 0, sizeof(stmt));
	init_input_buffer(&stmt, &in_buf, sizeof(in_buf));
	init_output_buffer(&stmt, &out_buf, sizeof(out_buf));

	stmt.stmt = sql_stmt;
	stmt.stmt_length = sizeof(GET_RES_MEMBERSHIP);
	in_buf.buffer_type = MYSQL_TYPE_STRING;
	out_buf.buffer_type = MYSQL_TYPE_LONG;

	return &stmt;
}

int_vector *get_res_membership(char *name)
{
	journal_strace("get_res_membership");

	struct stmt_singleton *stmt;
	stmt = get_res_membership_init_once();

	stmt->in_buf->buffer = name;
	stmt->in_buf->buffer_length = strlen(name);

	return mysql_exec(stmt);
}

#define GET_USR_MEMBERSHIP \
\
        "SELECT `acl_set` FROM `acl_sets_usr_map` WHERE `acl_group` = ANY " \
        "(SELECT `acl_group` FROM `acl_groups_map` WHERE `token` = XIA_TOKEN(?))"

struct stmt_singleton *get_usr_membership_init_once()
{
	journal_strace("get_usr_membership_init_once");

	static struct stmt_singleton stmt;
	static MYSQL_BIND in_buf;
	static MYSQL_BIND out_buf;
	static STMT_SQL_QUERY(sql_stmt, GET_USR_MEMBERSHIP);

	if (stmt.stmt_length == sizeof(GET_USR_MEMBERSHIP))
		return &stmt;

	memset(&stmt, 0, sizeof(stmt));
	init_input_buffer(&stmt, &in_buf, sizeof(in_buf));
	init_output_buffer(&stmt, &out_buf, sizeof(out_buf));

	stmt.stmt = sql_stmt;
	stmt.stmt_length = sizeof(GET_USR_MEMBERSHIP);
	in_buf.buffer_type = MYSQL_TYPE_STRING;
	out_buf.buffer_type = MYSQL_TYPE_LONG;

	return &stmt;
}

int_vector *get_usr_membership(char *name)
{
	journal_strace("get_usr_membership");

	struct stmt_singleton *stmt;
	stmt = get_usr_membership_init_once();

	stmt->in_buf->buffer = name;
	stmt->in_buf->buffer_length = strlen(name);

	return mysql_exec(stmt);
}

#define GET_BRIDGE_OWNER_CONTEXT \
\
        "SELECT `owned_by` FROM `br_tokens` WHERE `token` = XIA_TOKEN(?)"

struct stmt_singleton *get_bridge_owner_context_init_once()
{
	journal_strace("get_bridge_owner_context_init_once");

	static struct stmt_singleton stmt;
	static MYSQL_BIND in_buf;
	static MYSQL_BIND out_buf;
	static STMT_SQL_QUERY(sql_stmt, GET_BRIDGE_OWNER_CONTEXT);

	if (stmt.stmt_length == sizeof(GET_BRIDGE_OWNER_CONTEXT))
		return &stmt;

	memset(&stmt, 0, sizeof(stmt));
	init_input_buffer(&stmt, &in_buf, sizeof(in_buf));
	init_output_buffer(&stmt, &out_buf, sizeof(out_buf));

	stmt.stmt = sql_stmt;
	stmt.stmt_length = sizeof(GET_BRIDGE_OWNER_CONTEXT);
	in_buf.buffer_type = MYSQL_TYPE_STRING;
	out_buf.buffer_type = MYSQL_TYPE_SHORT;

	return &stmt;
}

int_vector *get_bridge_owner_context(char *name)
{
	journal_strace("get_bridge_owner_context");

	struct stmt_singleton *stmt;
	stmt = get_bridge_owner_context_init_once();

	stmt->in_buf->buffer = name;
	stmt->in_buf->buffer_length = strlen(name);

	return mysql_exec(stmt);
}

#define SET_TOKEN_ADDR_MAPPING \
\
        "INSERT INTO `tokens_addr_map` (`token`, `addr`) " \
        "VALUES (XIA_TOKEN(?), ?) ON DUPLICATE KEY UPDATE `addr` = ?"

enum {
	SET_TOKEN_ADDR_MAPPING_NAME = 0,
	SET_TOKEN_ADDR_MAPPING_ADDR,
	SET_TOKEN_ADDR_MAPPING_ADDR2,
	SET_TOKEN_ADDR_MAPPING_MAXBUF
};

struct stmt_singleton *set_token_addr_mapping_init_once()
{
	journal_strace("set_token_addr_mapping_init_once");

	static struct stmt_singleton stmt;
	static MYSQL_BIND in_buf[SET_TOKEN_ADDR_MAPPING_MAXBUF];
	static STMT_SQL_QUERY(sql_stmt, SET_TOKEN_ADDR_MAPPING);

	unsigned int i;

	if (stmt.stmt_length == sizeof(SET_TOKEN_ADDR_MAPPING))
		return &stmt;

	memset(&stmt, 0, sizeof(stmt));
	init_input_buffer(&stmt, &in_buf[0], sizeof(in_buf));

	stmt.stmt = sql_stmt;
	stmt.stmt_length = sizeof(SET_TOKEN_ADDR_MAPPING);

	for (i = 0; i < SET_TOKEN_ADDR_MAPPING_MAXBUF; ++i)
		in_buf[i].buffer_type = MYSQL_TYPE_STRING;

	return &stmt;
}

int set_token_addr_mapping(char *name, char *addr)
{
	journal_strace("set_token_addr_mapping");

	struct stmt_singleton *stmt;
	unsigned int i;
	MYSQL_BIND *p;
	
	stmt = set_token_addr_mapping_init_once();

	for (p = stmt->in_buf, i = 0;
	    i < SET_TOKEN_ADDR_MAPPING_MAXBUF; ++p, ++i) {
		switch (i) {
			case SET_TOKEN_ADDR_MAPPING_NAME:
				p->buffer = name;
				p->buffer_length = strlen(name);
				break;

			case SET_TOKEN_ADDR_MAPPING_ADDR:
				p->buffer = addr;
				p->buffer_length = strlen(addr);
				break;
		}
	}

	return (mysql_exec(stmt)) ? 1 : 0;
}

#define UNSET_TOKEN_ADDR_MAPPING \
\
        "DELETE FROM `tokens_addr_map` WHERE `addr` = ?"

struct stmt_singleton *unset_token_addr_mapping_init_once()
{
	journal_strace("unset_token_addr_mapping_init_once");

	static struct stmt_singleton stmt;
	static MYSQL_BIND in_buf;
	static STMT_SQL_QUERY(sql_stmt, UNSET_TOKEN_ADDR_MAPPING);

	if (stmt.stmt_length == sizeof(UNSET_TOKEN_ADDR_MAPPING))
		return &stmt;

	memset(&stmt, 0, sizeof(stmt));
	init_input_buffer(&stmt, &in_buf, sizeof(in_buf));

	stmt.stmt = sql_stmt;
	stmt.stmt_length = sizeof(UNSET_TOKEN_ADDR_MAPPING);

	in_buf.buffer_type = MYSQL_TYPE_STRING;

	return &stmt;
}

int unset_token_addr_mapping(char *addr)
{
	journal_strace("unset_token_addr_mapping");
	
	struct stmt_singleton *stmt;
	stmt = unset_token_addr_mapping_init_once();

	stmt->in_buf->buffer = addr;
	stmt->in_buf->buffer_length = strlen(addr);

	return (mysql_exec(stmt)) ? 1 : 0;
}

#define SET_RES_ADDR_MAPPING \
\
	"INSERT INTO `res_tokens` (`token`, `haddr`, `addr`) " \
	"VALUES (XIA_TOKEN(?), ?, ?) " \
	"ON DUPLICATE KEY UPDATE `addr` = ?"

enum {
	SET_RES_ADDR_MAPPING_NAME = 0,
	SET_RES_ADDR_MAPPING_HADDR,
	SET_RES_ADDR_MAPPING_ADDR,
	SET_RES_ADDR_MAPPING_ADDR2,
	SET_RES_ADDR_MAPPING_MAXBUF
};

struct stmt_singleton *set_res_addr_mapping_init_once()
{
	journal_strace("set_res_addr_mapping_init_once");

	static struct stmt_singleton stmt;
	static MYSQL_BIND in_buf[SET_RES_ADDR_MAPPING_MAXBUF];
	static STMT_SQL_QUERY(sql_stmt, SET_RES_ADDR_MAPPING);

	unsigned int i;

	if (stmt.stmt_length == sizeof(SET_RES_ADDR_MAPPING))
		return &stmt;
	
	memset(&stmt, 0, sizeof(stmt));
	init_input_buffer(&stmt, &in_buf[0], sizeof(in_buf));

	stmt.stmt = sql_stmt;
	stmt.stmt_length = sizeof(SET_RES_ADDR_MAPPING);

	for (i = 0; i < SET_RES_ADDR_MAPPING_MAXBUF; ++i)
		in_buf[i].buffer_type = MYSQL_TYPE_STRING;

	return &stmt;
}

int set_res_addr_mapping(char *name, char *haddr, char *addr)
{
	journal_strace("set_res_addr_mapping");

	struct stmt_singleton *stmt;
	MYSQL_BIND *p;
	unsigned int i;

	stmt = set_res_addr_mapping_init_once();

	for (p = stmt->in_buf, i = 0;
	    i < SET_RES_ADDR_MAPPING_MAXBUF; ++p, ++i) {
		switch (i) {
			case SET_RES_ADDR_MAPPING_NAME:
				p->buffer = name;
				p->buffer_length = strlen(name);
				break;

			case SET_RES_ADDR_MAPPING_HADDR:
				p->buffer = haddr;
				p->buffer_length = strlen(haddr);
				break;

			case SET_RES_ADDR_MAPPING_ADDR:
			case SET_RES_ADDR_MAPPING_ADDR2:
				p->buffer = addr;
				p->buffer_length = strlen(addr);
				break;
		}
	}

	return (mysql_exec(stmt)) ? 1 : 0;
}

#define SET_RES_HOSTNAME_MAPPING \
\
	"INSERT INTO `res_tokens` (`token`, `haddr`, `hostname`) " \
	"VALUES (XIA_TOKEN(?), ?, ?) " \
	"ON DUPLICATE KEY UPDATE `hostname` = ?"

enum {
	SET_RES_HOSTNAME_MAPPING_NAME = 0,
	SET_RES_HOSTNAME_MAPPING_HADDR,
	SET_RES_HOSTNAME_MAPPING_HOST,
	SET_RES_HOSTNAME_MAPPING_HOST2,
	SET_RES_HOSTNAME_MAPPING_MAXBUF
};

struct stmt_singleton *set_res_hostname_mapping_init_once()
{
	journal_strace("set_res_hostname_mapping_init_once");

	static struct stmt_singleton stmt;
	static MYSQL_BIND in_buf[SET_RES_HOSTNAME_MAPPING_MAXBUF];
	static STMT_SQL_QUERY(sql_stmt, SET_RES_HOSTNAME_MAPPING);

	unsigned int i;

	if (stmt.stmt_length == sizeof(SET_RES_HOSTNAME_MAPPING))
		return &stmt;

	memset(&stmt, 0, sizeof(stmt));
	init_input_buffer(&stmt, &in_buf[0], sizeof(in_buf));

	stmt.stmt = sql_stmt;
	stmt.stmt_length = sizeof(SET_RES_HOSTNAME_MAPPING);

	for (i = 0; i < SET_RES_HOSTNAME_MAPPING_MAXBUF; ++i)
		in_buf[i].buffer_type = MYSQL_TYPE_STRING;

	return &stmt;
}

int set_res_hostname_mapping(char *name, char *haddr, char *hostname)
{
	journal_strace("set_res_hostname_mapping");

	struct stmt_singleton *stmt;
	MYSQL_BIND *p;
	unsigned int i;

	stmt = set_res_hostname_mapping_init_once();

	for (p = stmt->in_buf, i = 0;
	    i < SET_RES_HOSTNAME_MAPPING_MAXBUF; ++p, ++i) {
		switch (i) {
			case SET_RES_HOSTNAME_MAPPING_NAME:
				p->buffer = name;
				p->buffer_length = strlen(name);
				break;

			case SET_RES_HOSTNAME_MAPPING_HADDR:
				p->buffer = haddr;
				p->buffer_length = strlen(haddr);
				break;

			case SET_RES_HOSTNAME_MAPPING_HOST:
			case SET_RES_HOSTNAME_MAPPING_HOST2:
				p->buffer = hostname;
				p->buffer_length = strlen(hostname);
				break;
		}
	}

	return (mysql_exec(stmt)) ? 1 : 0;
}

#define SET_RES_STATUS \
\
	"INSERT INTO `res_tokens` (`token`, `haddr`, `status`) " \
	"VALUES (XIA_TOKEN(?), ?, ?) " \
	"ON DUPLICATE KEY UPDATE `status` = ?"

enum {
	SET_RES_STATUS_NAME = 0,
	SET_RES_STATUS_HADDR,
	SET_RES_STATUS_STATUS,
	SET_RES_STATUS_STATUS2,
	SET_RES_STATUS_MAXBUF
};

struct stmt_singleton *set_res_status_init_once()
{
	journal_strace("set_res_status");

	static struct stmt_singleton stmt;
	static MYSQL_BIND in_buf[SET_RES_STATUS_MAXBUF];
	static STMT_SQL_QUERY(sql_stmt, SET_RES_STATUS);

	unsigned int i;

	if (stmt.stmt_length == sizeof(SET_RES_STATUS))
		return &stmt;

	memset(&stmt, 0, sizeof(stmt));
	init_input_buffer(&stmt, &in_buf[0], sizeof(in_buf));

	stmt.stmt = sql_stmt;
	stmt.stmt_length = sizeof(SET_RES_STATUS);

	in_buf[SET_RES_STATUS_NAME].buffer_type = MYSQL_TYPE_STRING;
	in_buf[SET_RES_STATUS_HADDR].buffer_type = MYSQL_TYPE_STRING;
	in_buf[SET_RES_STATUS_STATUS].buffer_type = MYSQL_TYPE_TINY;
	in_buf[SET_RES_STATUS_STATUS2].buffer_type = MYSQL_TYPE_TINY;

	return &stmt;
}

int set_res_status(char *name, char *haddr, unsigned int status)
{
	journal_strace("set_res_status");

	struct stmt_singleton *stmt;
	MYSQL_BIND *p;
	unsigned int i;

	stmt = set_res_status_init_once();

	for (p = stmt->in_buf, i = 0;
	    i < SET_RES_STATUS_MAXBUF; ++p, ++i) {
		switch (i) {
			case SET_RES_STATUS_NAME:
				p->buffer = name;
				p->buffer_length = strlen(name);
				break;

			case SET_RES_STATUS_HADDR:
				p->buffer = haddr;
				p->buffer_length = strlen(haddr);
				break;

			case SET_RES_STATUS_STATUS:
			case SET_RES_STATUS_STATUS2:
				p->buffer = (char *)&status;
				break;
		}
	}

	return (mysql_exec(stmt)) ? 1 : 0;
}

int_vector *mysql_exec(struct stmt_singleton *stmt)
{
	journal_strace("mysql_exec");

	int ret, num_row, itr;
	int_vector *data_vtr;

	static int buf = 0;	/* XXX being static fixes a bad bug */

	printf("Running MySQL query\n\n\t%s\n\n", stmt->stmt);

	if (stmt->out_buf_count > 1) {
		journal_notice("%s :: %s:%i\n",
			"mysql_exec() does not support multicolumn vector",
			__FILE__, __LINE__);
		return NULL;
	}

	ret = mysql_stmt_prepare(mysql_stmt, stmt->stmt, stmt->stmt_length);
	printf("mysql_stmt_prepare (%i)\n", ret);

	ret = mysql_stmt_bind_param(mysql_stmt, stmt->in_buf);
	printf("mysql_stmt_bind_param (%i)\n", ret);

	ret = mysql_stmt_param_count(mysql_stmt);
	printf("mysql_stmt_param_count (%i)\n", ret);

	if (ret != stmt->in_buf_count) {
		journal_notice("%s :: %s:%i\n",
			"corrupted dbal method", __FILE__, __LINE__);
		return NULL;
	}

	if (stmt->out_buf) {
		stmt->out_buf->buffer = (char *)&buf;
		ret = mysql_stmt_bind_result(mysql_stmt, stmt->out_buf);
		printf("mysql_stmt_bind_result (%i)\n", ret);
	}

	ret = mysql_stmt_execute(mysql_stmt);
	printf("mysql_stmt_execute (%i)\n", ret);

	if (ret == 1) { // I dont know why...
		printf("err: %s\n", mysql_stmt_error(mysql_stmt));
		return NULL;
	}

	ret = mysql_stmt_store_result(mysql_stmt);
	printf("mysql_stmt_store_result (%i)\n", ret);

	num_row = mysql_stmt_num_rows(mysql_stmt);
	printf("mysql_stmt_num_rows (%i)\n", num_row);

	data_vtr = calloc(VECTOR_SET_MAX(num_row), sizeof(int_vector));
	data_vtr[VECTOR_IDX_MAX] = num_row;

	for (itr = VECTOR_IDX_BEGIN; !mysql_stmt_fetch(mysql_stmt); itr++) {
		data_vtr[itr] = buf;
		printf("MySQL returned value %i\n", buf);
	}
	
	data_vtr[VECTOR_IDX_SIZE] = VECTOR_SET_SIZE(itr);
	
	return data_vtr;	
}

int db_init(const char *host, const char *user,
	    const char *passwd, const char *db)
{
	journal_strace("db_init");

	static const unsigned timeout = 1;
	int err = 0;

	if (mysql_init(&mysql) == NULL) {
		printf("mysql_init() %s :: %s:%i\n", 
			mysql_error(&mysql), __FILE__, __LINE__);

		return -1;
	}

	err = mysql_options(&mysql, MYSQL_OPT_CONNECT_TIMEOUT,
		(const char *)&timeout);

	if (err) {
		mysql_close(&mysql);
		printf("mysql_options() %s :: %s:%i\n",
			mysql_error(&mysql), __FILE__, __LINE__);

		return -1;
	}

	// XXX - OPT_RECONNECT

	if (mysql_real_connect(&mysql, host, user, passwd,
			       db, 0, NULL, 0) == NULL)
       	{
		mysql_close(&mysql);
		printf("mysql_real_connect() %s :: %s:%i\n",
			mysql_error(&mysql), __FILE__, __LINE__);
		
		return -1;
	}

	mysql_stmt = mysql_stmt_init(&mysql);
	// XXX handle mysql_stmt return value
	
	return 0;
}

int db_fini()
{
	journal_strace("db_fini");

	return 0;
}

int hookin()
{
	journal_strace("hookin");

	return HOOKLET_DBAL;
}
