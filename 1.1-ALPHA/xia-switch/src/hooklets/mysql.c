/*
 * See COPYRIGHTS file.
 */


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <mysql/mysql.h>
#include <mysql/errmsg.h>

#include <xia/hooklet.h>
#include <xia/journal.h>
#include <xia/utils.h>

#define MYSQL_BIND_SIZE sizeof(MYSQL_BIND)
#define STMT_SQL_QUERY(name, sql) char name[sizeof(sql)] = sql

MYSQL		mysql;
MYSQL_STMT     *mysql_stmt;

struct stmt_singleton {
	char		*stmt;		/* SQL statement */
	size_t		stmt_length;	/* Length of SQL statement */

	MYSQL_BIND	*in_buf;	/* MySQL meta structure for input buffers */
	uint8_t		in_buf_count;	/* Number of input buffers */
	MYSQL_BIND	*out_buf;	/* MySQL meta structure for output buffers */
	uint8_t		out_buf_count;	/* Number of output buffers */
};

void init_input_buffer(struct stmt_singleton *s, MYSQL_BIND *buf, size_t buf_size)
{
	journal_ftrace(__func__);

	s->in_buf = buf;
	s->in_buf_count = buf_size / MYSQL_BIND_SIZE;
	memset(buf, 0, buf_size);
}

void init_output_buffer(struct stmt_singleton *s, MYSQL_BIND *buf, size_t buf_size)
{
	journal_ftrace(__func__);

	s->out_buf = buf;
	s->out_buf_count = buf_size / MYSQL_BIND_SIZE;
	memset(buf, 0, buf_size);
}

int mysql_exec(struct stmt_singleton *stmt)
{
	journal_ftrace(__func__);

	int ret, num_row;

	journal_notice("Running MySQL query\n\n\t%s\n\n", stmt->stmt);

	ret = mysql_stmt_prepare(mysql_stmt, stmt->stmt, stmt->stmt_length);
	if (ret != 0) {
		journal_notice("mysql]> mysql_stmt_prepare failed, %s :: %s:%i\n",
		    mysql_stmt_error(mysql_stmt), __FILE__, __LINE__);
		return -1;
	}

	if (stmt->in_buf) {
		ret = mysql_stmt_bind_param(mysql_stmt, stmt->in_buf);
		if (ret != 0) {
			journal_notice("mysql]> mysql_stmt_bind_param failed, %s :: %s:%i\n",
			    mysql_stmt_error(mysql_stmt), __FILE__, __LINE__);
			return -1;
		}

		ret = mysql_stmt_param_count(mysql_stmt);
		if (ret != stmt->in_buf_count) {
			journal_notice("mysql]> corrupted dbal method :: %s:%i\n",
			    __FILE__, __LINE__);
			return -1;
		}
	}

	if (stmt->out_buf) {
		ret = mysql_stmt_bind_result(mysql_stmt, stmt->out_buf);
		if (ret != 0) {
			journal_notice("mysql]> mysql_stmt_bind_param failed %s :: %s:%i\n",
			    mysql_stmt_error(mysql_stmt), __FILE__, __LINE__);
			return -1;
		}

	}

	ret = mysql_stmt_execute(mysql_stmt);
	if (ret != 0) {
		journal_notice("mysql]> mysql_stmt_execute failed, %s :: %s:%i\n",
		    mysql_stmt_error(mysql_stmt), __FILE__, __LINE__);
		return -1;
	}

	ret = mysql_stmt_store_result(mysql_stmt);
	if (ret != 0) {
		journal_notice("mysql]> mysql_stmt_store_result failed, %s :: %s:%i\n",
		    mysql_stmt_error(mysql_stmt), __FILE__, __LINE__);
		return -1;
	}

	num_row = mysql_stmt_num_rows(mysql_stmt);
	journal_notice("mysql_stmt_num_rows (%i)\n", num_row);

	return num_row;
}

/*
 * get_context_pool method
 */
#define GET_CONTEXT_POOL \
\
	"SELECT `contexts`.`id`, `pools`.`local`, `pools`.`begin`, `pools`.`end`, `pools`.`netmask` " \
	"FROM `contexts` LEFT JOIN `pools` ON `contexts`.`pool` = `pools`.`id` " \
	"WHERE `contexts`.`id` LIKE ?"

enum {
	GET_CONTEXT_POOL_OUT_CONTEXT = 0,
	GET_CONTEXT_POOL_OUT_LOCAL,
	GET_CONTEXT_POOL_OUT_BEGIN,
	GET_CONTEXT_POOL_OUT_END,
	GET_CONTEXT_POOL_OUT_NETMASK,
	SIZEOF_GET_CONTEXT_POOL_OUT
};

#define POOL_ADDR_LEN 15 + 1
struct stmt_singleton *get_context_pool_init_once()
{
	journal_ftrace(__func__);

	static struct stmt_singleton stmt;
	static MYSQL_BIND in_buf;
	static MYSQL_BIND out_buf[SIZEOF_GET_CONTEXT_POOL_OUT];
	static STMT_SQL_QUERY(sql_stmt, GET_CONTEXT_POOL);

	if (stmt.stmt_length == sizeof(GET_CONTEXT_POOL))
		return &stmt;

	memset(&stmt, 0, sizeof(stmt));
	init_input_buffer(&stmt, &in_buf, sizeof(in_buf));
	init_output_buffer(&stmt, out_buf, sizeof(out_buf));

	stmt.stmt = sql_stmt;
	stmt.stmt_length = sizeof(GET_CONTEXT_POOL);

	in_buf.buffer_type = MYSQL_TYPE_STRING;

	out_buf[GET_CONTEXT_POOL_OUT_CONTEXT].buffer_type = MYSQL_TYPE_SHORT;

	out_buf[GET_CONTEXT_POOL_OUT_LOCAL].buffer_type = MYSQL_TYPE_STRING;
	out_buf[GET_CONTEXT_POOL_OUT_LOCAL].buffer_length = POOL_ADDR_LEN;

	out_buf[GET_CONTEXT_POOL_OUT_BEGIN].buffer_type = MYSQL_TYPE_STRING;
	out_buf[GET_CONTEXT_POOL_OUT_BEGIN].buffer_length = POOL_ADDR_LEN;

	out_buf[GET_CONTEXT_POOL_OUT_END].buffer_type = MYSQL_TYPE_STRING;
	out_buf[GET_CONTEXT_POOL_OUT_END].buffer_length = POOL_ADDR_LEN;

	out_buf[GET_CONTEXT_POOL_OUT_NETMASK].buffer_type = MYSQL_TYPE_STRING;
	out_buf[GET_CONTEXT_POOL_OUT_NETMASK].buffer_length = POOL_ADDR_LEN;

	return &stmt;
}

int get_context_pool(int id, void (*cb)(char *, char *, char *, char *))
{
	journal_ftrace(__func__);

	char	local[POOL_ADDR_LEN],
		begin[POOL_ADDR_LEN],
		end[POOL_ADDR_LEN],
		netmask[POOL_ADDR_LEN];

	int i, ret;
	MYSQL_BIND *p;

	struct stmt_singleton *stmt;
	stmt = get_context_pool_init_once();

	stmt->in_buf->buffer = (char *)&id;

	for (p = stmt->out_buf, i = 0;
	    i < SIZEOF_GET_CONTEXT_POOL_OUT; ++p, ++i)
		switch (i) {
			case GET_CONTEXT_POOL_OUT_LOCAL:
				p->buffer = local;
				break;

			case GET_CONTEXT_POOL_OUT_BEGIN:
				p->buffer = begin;
				break;

			case GET_CONTEXT_POOL_OUT_END:
				p->buffer = end;
				break;

			case GET_CONTEXT_POOL_OUT_NETMASK:
				p->buffer = netmask;
				break;
		}

	ret = mysql_exec(stmt);

	if (ret > 0)
		while (!mysql_stmt_fetch(mysql_stmt))
			cb(local, begin, end, netmask);

	return ret;
}

int get_all_context_pool(void (*cb)(unsigned short, char *, char *, char *, char *))
{
	journal_ftrace(__func__);

	char all[] = "%";

	char	local[POOL_ADDR_LEN],
		begin[POOL_ADDR_LEN],
		end[POOL_ADDR_LEN],
		netmask[POOL_ADDR_LEN];

	unsigned short context;

	int i, ret;
	MYSQL_BIND *p;

	struct stmt_singleton *stmt;
	stmt = get_context_pool_init_once();

	stmt->in_buf->buffer = all;
	stmt->in_buf->buffer_length = strlen(all);

	for (p = stmt->out_buf, i = 0;
	    i < SIZEOF_GET_CONTEXT_POOL_OUT; ++p, ++i)
		switch (i) {
			case GET_CONTEXT_POOL_OUT_CONTEXT:
				p->buffer = (char *)&context;
				break;

			case GET_CONTEXT_POOL_OUT_LOCAL:
				p->buffer = local;
				break;

			case GET_CONTEXT_POOL_OUT_BEGIN:
				p->buffer = begin;
				break;

			case GET_CONTEXT_POOL_OUT_END:
				p->buffer = end;
				break;

			case GET_CONTEXT_POOL_OUT_NETMASK:
				p->buffer = netmask;
				break;
		}

	ret = mysql_exec(stmt);

	if (ret > 0)
		while (!mysql_stmt_fetch(mysql_stmt))
			cb(context, local, begin, end, netmask);

	return ret;
}

/*
 * get_token_context
 */
#define GET_TOKEN_CONTEXT \
\
	"SELECT `context` FROM `tokens` WHERE `name` = XIA_TOKEN(?) AND `context` <> 0"

struct stmt_singleton *get_token_context_init_once()
{
	journal_ftrace(__func__);

	static struct stmt_singleton stmt;
	static MYSQL_BIND in_buf;
	static MYSQL_BIND out_buf;
	static STMT_SQL_QUERY(sql_stmt, GET_TOKEN_CONTEXT);

	if (stmt.stmt_length == sizeof(GET_TOKEN_CONTEXT))
		return &stmt;

	memset(&stmt, 0, sizeof(stmt));
	init_input_buffer(&stmt, &in_buf, sizeof(in_buf));
	init_output_buffer(&stmt, &out_buf, sizeof(out_buf));

	stmt.stmt = sql_stmt;
	stmt.stmt_length = sizeof(GET_TOKEN_CONTEXT);
	in_buf.buffer_type = MYSQL_TYPE_STRING;
	out_buf.buffer_type = MYSQL_TYPE_SHORT;

	return &stmt;
}

int get_token_context(char *name)
{
	journal_ftrace(__func__);

	struct stmt_singleton *stmt;
	stmt = get_token_context_init_once();

	stmt->in_buf->buffer = name;
	stmt->in_buf->buffer_length = strlen(name);

	return mysql_exec(stmt);
}

char *db_host, *db_user, *db_passwd, *db_name;

int db_ping()
{
	journal_ftrace(__func__);

	int ret;

	ret = mysql_ping(&mysql);
	if (ret != 0) {
		/* we've got disconnected...  try to reconnect */
		if (mysql_real_connect(&mysql, db_host, db_user,
		    db_passwd, db_name, 0, NULL, 0) == NULL) {
			journal_notice("mysql]> mysql_real_connect failed, %s :: %s:%i\n",
			    mysql_error(&mysql), __FILE__, __LINE__);
			return -1;
		}
	}
	/* we are still connected... */
	return 0;
}

int db_init(const char *host, const char *user, const char *passwd, const char *db)
{
	journal_ftrace(__func__);

	static const unsigned timeout = 50;
	int err = 0;
	my_bool	reconnect = 1;

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

	if (mysql_real_connect(&mysql, host, user, passwd, db, 0, NULL, 0) == NULL) {
		mysql_close(&mysql);
		 journal_notice("mysql]> %s :: %s:%i\n", mysql_error(&mysql), __FILE__, __LINE__);
		return -1;
	}

	mysql_stmt = mysql_stmt_init(&mysql);

	db_host = strdup(host);
	db_user = strdup(user);
	db_passwd = strdup(passwd);
	db_name = strdup(db);

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
