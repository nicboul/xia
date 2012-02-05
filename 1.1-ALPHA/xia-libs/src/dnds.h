/*
 * See COPYRIGHTS file.
 */


#ifndef XIA_DNDS_H
#define XIA_DNDS_H

#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>

#define DNDS_VERSION	1

/*
 * DNDS specification
 */
#define DNDS_VERSION_MAX 0xffff
#define DNDS_P_SIZE_MAX 0xffff
#define DNDS_ERROR_MAX 0xff

#define DNDS_SUBJECT_MAX 0xffff
enum {
	DNDS_HELLO = 0,
	DNDS_GOODBYE,

	/* ACL */
	DNDS_ACL_GET,
	DNDS_ACL_LIST,
	DNDS_ACL_NEW,
	DNDS_ACL_EDIT,
	DNDS_ACL_CLEAR,
	DNDS_ACL_DELETE,
	DNDS_ACL_SRC_GROUP_MEMBER,
	DNDS_ACL_SRC_GROUP_NOT_MEMBER,
	DNDS_ACL_SRC_GROUP_MAP,
	DNDS_ACL_SRC_GROUP_UNMAP,
	DNDS_ACL_SRC_HOST_MEMBER,
	DNDS_ACL_SRC_HOST_NOT_MEMBER,
	DNDS_ACL_SRC_HOST_MAP,
	DNDS_ACL_SRC_HOST_UNMAP,
	DNDS_ACL_DST_GROUP_MEMBER,
	DNDS_ACL_DST_GROUP_NOT_MEMBER,
	DNDS_ACL_DST_GROUP_MAP,
	DNDS_ACL_DST_GROUP_UNMAP,
	DNDS_ACL_DST_HOST_MEMBER,
	DNDS_ACL_DST_HOST_NOT_MEMBER,
	DNDS_ACL_DST_HOST_MAP,
	DNDS_ACL_DST_HOST_UNMAP,

	/* ACL_GROUP */
	DNDS_ACL_GROUP_GET,
	DNDS_ACL_GROUP_LIST,
	DNDS_ACL_GROUP_NEW,
	DNDS_ACL_GROUP_EDIT,
	DNDS_ACL_GROUP_CLEAR,
	DNDS_ACL_GROUP_DELETE,
	DNDS_ACL_GROUP_HOST_MEMBER,
	DNDS_ACL_GROUP_HOST_NOT_MEMBER,
	DNDS_ACL_GROUP_HOST_MAP,
	DNDS_ACL_GROUP_HOST_UNMAP,

	/* ADDR_POOL */
	DNDS_ADDR_POOL_GET,
	DNDS_ADDR_POOL_LIST,
	DNDS_ADDR_POOL_NEW,
	DNDS_ADDR_POOL_EDIT,
	DNDS_ADDR_POOL_CLEAR,
	DNDS_ADDR_POOL_DELETE,

	/* CONTEXT */
	DNDS_CONTEXT_GET,
	DNDS_CONTEXT_LIST,
	DNDS_CONTEXT_NEW,
	DNDS_CONTEXT_EDIT,
	DNDS_CONTEXT_CLEAR,
	DNDS_CONTEXT_DELETE,

	/* HOST */
	DNDS_HOST_GET,
	DNDS_HOST_LIST,
	DNDS_HOST_NEW,
	DNDS_HOST_EDIT,
	DNDS_HOST_CLEAR,
	DNDS_HOST_DELETE,

	/* NODE */
	DNDS_NODE_GET,
	DNDS_NODE_LIST,
	DNDS_NODE_NEW,
	DNDS_NODE_EDIT,
	DNDS_NODE_CLEAR,
	DNDS_NODE_DELETE,

	/* PEER */
	DNDS_PEER_GET,
	DNDS_PEER_LIST,
	DNDS_PEER_NEW,
	DNDS_PEER_EDIT,
	DNDS_PEER_CLEAR,
	DNDS_PEER_DELETE,
	DNDS_PEER_USER_MEMBER,
	DNDS_PEER_USER_NOT_MEMBER,
	DNDS_PEER_USER_MAP,
	DNDS_PEER_USER_UNMAP,
	DNDS_PEER_HOST_MEMBER,
	DNDS_PEER_HOST_NOT_MEMBER,
	DNDS_PEER_HOST_MAP,
	DNDS_PEER_HOST_UNMAP,

	/* PERM */
	DNDS_PERM_GET,
	DNDS_PERM_LIST,
	DNDS_PERM_NEW,
	DNDS_PERM_EDIT,
	DNDS_PERM_CLEAR,
	DNDS_PERM_DELETE,

	/* USER */
	DNDS_USER_GET,
	DNDS_USER_LIST,
	DNDS_USER_NEW,
	DNDS_USER_EDIT,
	DNDS_USER_CLEAR,
	DNDS_USER_DELETE,

	DNDS_SUBJECT_COUNT
};

/* Status number */
#define DNDS_STATUS_MAX 0xff
enum {
	DNDS_STATUS_FAILED = 0,	/* operation failed */
	DNDS_STATUS_OK,		/* successful answer */
	DNDS_STATUS_QUERY,	/* client request */
	DNDS_STATUS_DENIED,	/* query is forbidden */
	DNDS_STATUS_NOLISTEN,	/* no such listener */
	DNDS_STATUS_UNEXPECTED,	/* unexpected error */

	DNDS_STATUS_COUNT
};

/* Flags */
#define DNDS_FLAG_TRUNC	0x1	/* more data to come */
#define DNDS_FLAG_MAX	0xff

struct dnds {
	uint16_t version;	/* version number */
	uint16_t subject;	/* subject number */
	uint16_t p_size;	/* payload size */
	uint8_t flag;		/* flags */
	uint8_t status;		/* status number */
	uint8_t error;		/* error number */

	/* XXX: missing 24bit padding */
};
#define DNDS_HEADER_SIZE sizeof(struct dnds)

static inline void dnds_hton(struct dnds *dnds)
{
	dnds->version = htons(dnds->version);
	dnds->subject = htons(dnds->subject);
	dnds->p_size = htons(dnds->p_size);
}

static inline void dnds_ntoh(struct dnds *dnds)
{
	dnds->version = ntohs(dnds->version);
	dnds->subject = ntohs(dnds->subject);
	dnds->p_size = ntohs(dnds->p_size);
}

/*
 * DNDS_ACL specification
 */

#define DNDS_ACL_ID_MAX 0xffffffff
#define DNDS_ACL_CONTEXT_MAX 0xffff	/* DNDS_CONTEXT_ID_MAX */
#define DNDS_ACL_DESC_SIZE 256

#define DNDS_ACL_AGE_MAX 0xffffffff
typedef struct {
	uint32_t id;				/* identifier */
	uint16_t context;			/* context */
	char desc[DNDS_ACL_DESC_SIZE];		/* description */
	uint32_t age;				/* age */
} __attribute__((__packed__)) DNDS_ACL;

enum {
	DNDS_ACL_ID = 0,
	DNDS_ACL_CONTEXT,
	DNDS_ACL_DESC,
	DNDS_ACL_AGE,

	END_DNDS_ACL
};

static inline void dnds_acl_hton(DNDS_ACL *obj)
{
	obj->id = htonl(obj->id);
	obj->context = htons(obj->context);
	obj->age = htonl(obj->age);
}

static inline void dnds_acl_ntoh(DNDS_ACL *obj)
{
	obj->id = ntohl(obj->id);
	obj->context = ntohs(obj->context);
	obj->age = ntohl(obj->age);
}

extern void dnds_acl_print(DNDS_ACL *);

/*
 * DNDS_ACL_GROUP specification
 */

#define DNDS_ACL_GROUP_ID_MAX 0xffffffff
#define DNDS_ACL_GROUP_CONTEXT_MAX 0xffff	/* DNDS_CONTEXT_ID_MAX */
#define DNDS_ACL_GROUP_NAME_SIZE 128
#define DNDS_ACL_GROUP_DESC_SIZE 256
#define DNDS_ACL_GROUP_AGE_MAX 0xffffffff
typedef struct {
	uint32_t id;				/* identifier */
	uint16_t context;			/* context */
	char name[DNDS_ACL_GROUP_NAME_SIZE];	/* name */
	char desc[DNDS_ACL_GROUP_DESC_SIZE];	/* description */
	uint32_t age;				/* age */
} __attribute__((__packed__)) DNDS_ACL_GROUP;

enum {
	DNDS_ACL_GROUP_ID = 0,
	DNDS_ACL_GROUP_CONTEXT,
	DNDS_ACL_GROUP_NAME,
	DNDS_ACL_GROUP_DESC,
	DNDS_ACL_GROUP_AGE,

	END_DNDS_ACL_GROUP
};

static inline void dnds_acl_group_hton(DNDS_ACL_GROUP *obj)
{
	obj->id = htonl(obj->id);
	obj->context = htons(obj->context);
	obj->age = htonl(obj->age);
}

static inline void dnds_acl_group_ntoh(DNDS_ACL_GROUP *obj)
{
	obj->id = ntohl(obj->id);
	obj->context = ntohs(obj->context);
	obj->age = ntohl(obj->age);
}

/*
 * DNDS_ADDR_POOL specification
 */

#define DNDS_ADDR_POOL_ID_MAX 0xffffffff
#define DNDS_ADDR_POOL_DESC_SIZE 256
#define DNDS_ADDR_POOL_AGE_MAX 0xffffffff

typedef struct {
	uint32_t id;				/* identifier */
	struct in_addr local;			/* local ip address */
	struct in_addr begin;			/* pool begins at */
	struct in_addr end;			/* pool ends at */
	struct in_addr netmask;			/* netmask */
	char desc[DNDS_ADDR_POOL_DESC_SIZE];	/* description */
	uint32_t age;				/* age */
} __attribute__((__packed__)) DNDS_ADDR_POOL;

enum {
	DNDS_ADDR_POOL_ID = 0,
	DNDS_ADDR_POOL_LOCAL,
	DNDS_ADDR_POOL_BEGIN,
	DNDS_ADDR_POOL_END,
	DNDS_ADDR_POOL_NETMASK,
	DNDS_ADDR_POOL_DESC,
	DNDS_ADDR_POOL_AGE,

	END_DNDS_ADDR_POOL
};

static inline void dnds_addr_pool_hton(DNDS_ADDR_POOL *obj)
{
	obj->id = htonl(obj->id);
	obj->age = htonl(obj->age);
}

static inline void dnds_addr_pool_ntoh(DNDS_ADDR_POOL *obj)
{
	obj->id = ntohl(obj->id);
	obj->age = ntohl(obj->age);
}

/*
 * DNDS_CONTEXT specification
 */

#define DNDS_CONTEXT_ID_MAX 0xffff
#define DNDS_CONTEXT_DNS_ZONE_SIZE 128
#define DNDS_CONTEXT_DNS_SERIAL_MAX 0xffffffff
#define DNDS_CONTEXT_VHOST_SIZE 128
#define DNDS_CONTEXT_DESC_SIZE 256
#define DNDS_CONTEXT_AGE_MAX 0xffffffff

typedef struct {
	uint16_t id;					/* identifier */
	char dns_zone[DNDS_CONTEXT_DNS_ZONE_SIZE];	/* dns zone */
	uint32_t dns_serial;				/* dns zone serial */
	char vhost[DNDS_CONTEXT_VHOST_SIZE];		/* virtual hostname */
	char desc[DNDS_CONTEXT_DESC_SIZE];		/* description */
	uint32_t age;					/* age */
} __attribute__((__packed__)) DNDS_CONTEXT;

enum {
	DNDS_CONTEXT_ID = 0,
	DNDS_CONTEXT_DNS_ZONE,
	DNDS_CONTEXT_DNS_SERIAL,
	DNDS_CONTEXT_VHOST,
	DNDS_CONTEXT_DESC,
	DNDS_CONTEXT_AGE,

	END_DNDS_CONTEXT
};

static inline void dnds_context_hton(DNDS_CONTEXT *obj)
{
	obj->id = htons(obj->id);
	obj->dns_serial = htonl(obj->dns_serial);
	obj->age = htonl(obj->age);
}

static inline void dnds_context_ntoh(DNDS_CONTEXT *obj)
{
	obj->id = ntohs(obj->id);
	obj->dns_serial = ntohl(obj->dns_serial);
	obj->age = ntohl(obj->age);
}

/*
 * DNDS_HOST specification
 */

#define DNDS_HOST_ID_MAX 0xffffffff
#define DNDS_HOST_CONTEXT_MAX 0xffff	/* DNDS_CONTEXT_ID_MAX */
#define DNDS_HOST_PEER_MAX 0xffffffff	/* DNDS_PEER_ID_MAX */
#define DNDS_HOST_NAME_SIZE 64
#define DNDS_HOST_HADDR_SIZE 6

#define DNDS_HOST_FLAG_DISABLED		0x01	/* host is disabled */
#define DNDS_HOST_FLAG_DOWN		0x02	/* host is down */
#define DNDS_HOST_FLAG_MAX		0xff

#define DNDS_HOST_AGE_MAX 0xffffffff

typedef struct {
	uint32_t id;				/* identifier */
	uint16_t context;			/* context */
	uint32_t peer;				/* peer identifier */
	char name[DNDS_HOST_NAME_SIZE];		/* hostname */
	uint8_t haddr[DNDS_HOST_HADDR_SIZE];	/* hardware address */
	struct in_addr addr;			/* ip address */
	uint8_t flag;				/* flags */
	uint32_t age;				/* age */
} __attribute__((__packed__)) DNDS_HOST;

enum {
	DNDS_HOST_ID = 0,
	DNDS_HOST_CONTEXT,
	DNDS_HOST_PEER,
	DNDS_HOST_NAME,
	DNDS_HOST_HADDR,
	DNDS_HOST_ADDR,
	DNDS_HOST_FLAG,
	DNDS_HOST_AGE,

	END_DNDS_HOST
};

static inline void dnds_host_hton(DNDS_HOST *obj)
{
	obj->id = htonl(obj->id);
	obj->context = htons(obj->context);
	obj->peer = htonl(obj->peer);
	obj->age = htonl(obj->age);
}

static inline void dnds_host_ntoh(DNDS_HOST *obj)
{
	obj->id = ntohl(obj->id);
	obj->context = ntohs(obj->context);
	obj->peer = ntohl(obj->peer);
	obj->age = ntohl(obj->age);
}

/*
 * DNDS_NODE specification
 */

#define DNDS_NODE_ID_MAX 0xffffffff
#define DNDS_NODE_NAME_SIZE 64

#define DNDS_NODE_TYPE_MAX 0xff
enum {
	DNDS_NODE_TYPE_DNDS = 0,		/* dnds node */
	DNDS_NODE_TYPE_SWITCH,			/* switch node */
	DNDS_NODE_TYPE_MGMT,			/* management node */

	DNDS_NODE_TYPE_COUNT
};

#define DNDS_NODE_PERM_MAX 0xffffffff	/* DNDS_PERM_ID_MAX */

#define DNDS_NODE_FLAG_DISABLED		0x01	/* node is disabled */
#define DNDS_NODE_FLAG_DOWN		0x02	/* node is down */
#define DNDS_NODE_FLAG_ROOT		0x04	/* node is root */
#define DNDS_NODE_FLAG_MAX		0xff

#define DNDS_NODE_AGE_MAX 0xffffffff

typedef struct {
	uint32_t id;				/* identifier */
	uint8_t type;				/* type */
	char name[DNDS_NODE_NAME_SIZE];		/* name */
	struct in_addr addr;			/* ip address */
	uint32_t perm;				/* perm identifier */
	uint8_t flag;				/* flags */
	uint32_t age;				/* age */
} __attribute__((__packed__)) DNDS_NODE;

enum {
	DNDS_NODE_ID = 0,
	DNDS_NODE_TYPE,
	DNDS_NODE_NAME,
	DNDS_NODE_ADDR,
	DNDS_NODE_PERM,
	DNDS_NODE_FLAG,
	DNDS_NODE_AGE,

	END_DNDS_NODE
};

static inline void dnds_node_hton(DNDS_NODE *obj)
{
	obj->id = htonl(obj->id);
	obj->perm = htonl(obj->perm);
	obj->age = htonl(obj->age);
}

static inline void dnds_node_ntoh(DNDS_NODE *obj)
{
	obj->id = ntohl(obj->id);
	obj->perm = ntohl(obj->perm);
	obj->age = ntohl(obj->age);
}

extern void dnds_node_print(DNDS_NODE *);

/*
 * DNDS_PERM specification
 */
#define DNDS_PERM_ID_MAX 0xffffffff
#define DNDS_PERM_NAME_SIZE 64
#define DNDS_PERM_MATRIX_MAX DNDS_SUBJECT_COUNT	/* bitvector please! */
#define DNDS_PERM_AGE_MAX 0xffffffff

typedef struct {
	uint32_t id;				/* identifier */
	char name[DNDS_PERM_NAME_SIZE];		/* name */
	uint8_t matrix[DNDS_PERM_MATRIX_MAX];	/* permission matrix */
	uint32_t age;				/* age */
} __attribute__((__packed__)) DNDS_PERM;

enum {
	DNDS_PERM_ID = 0,
	DNDS_PERM_NAME,
	DNDS_PERM_MATRIX,
	DNDS_PERM_AGE,

	END_DNDS_PERM
};

static inline void dnds_perm_hton(DNDS_PERM *obj)
{
	obj->id = htonl(obj->id);
	obj->age = htonl(obj->age);
}

static inline void dnds_perm_ntoh(DNDS_PERM *obj)
{
	obj->id = ntohl(obj->id);
	obj->age = ntohl(obj->age);
}

extern void dnds_perm_print(DNDS_PERM *);

/*
 * DNDS_PEER specification
 */

#define DNDS_PEER_ID_MAX 0xffffffff
#define DNDS_PEER_CONTEXT_MAX 0xffff	/* DNDS_CONTEXT_ID_MAX */
#define DNDS_PEER_NAME_SIZE 64

#define DNDS_PEER_FLAG_DISABLED	0x01	/* peer is disabled */
#define DNDS_PEER_FLAG_MAX	0xff

#define DNDS_PEER_AGE_MAX 0xffffffff

typedef struct {
	uint32_t id;				/* identifier */
	uint16_t context;			/* context */
	char name[DNDS_PEER_NAME_SIZE];		/* name */
	struct in_addr addr;			/* ip address */
	uint8_t flag;				/* flags */
	uint32_t age;				/* age */
} __attribute__((__packed__)) DNDS_PEER;

enum {
	DNDS_PEER_ID = 0,
	DNDS_PEER_CONTEXT,
	DNDS_PEER_NAME,
	DNDS_PEER_ADDR,
	DNDS_PEER_FLAG,
	DNDS_PEER_AGE,

	END_DNDS_PEER
};

static inline void dnds_peer_hton(DNDS_PEER *obj)
{
	obj->id = htonl(obj->id);
	obj->context = htons(obj->context);
	obj->age = htonl(obj->age);
}

static inline void dnds_peer_ntoh(DNDS_PEER *obj)
{
	obj->id = ntohl(obj->id);
	obj->context = ntohs(obj->context);
	obj->age = ntohl(obj->age);
}

/*
 * DNDS_USER specification
 */

#define DNDS_USER_ID_MAX 0xffffffff
#define DNDS_USER_CONTEXT_MAX 0xffff	/* DNDS_CONTEXT_ID_MAX */
#define DNDS_USER_NAME_SIZE 45
#define DNDS_USER_PASSWORD_SIZE 32
#define DNDS_USER_FIRSTNAME_SIZE 64
#define DNDS_USER_LASTNAME_SIZE 64
#define DNDS_USER_EMAIL_SIZE 256
#define DNDS_USER_ROLE_MAX 0xff

#define DNDS_USER_FLAG_DISABLED		0x01	/* user is disabled */
#define DNDS_USER_FLAG_MAX		0xff

#define DNDS_USER_AGE_MAX 0xffffffff

typedef struct {
	uint32_t id;				/* identifier */
	uint16_t context;			/* context */
	char name[DNDS_USER_NAME_SIZE];		/* name */
	char password[DNDS_USER_PASSWORD_SIZE];	/* password */
	char firstname[DNDS_USER_FIRSTNAME_SIZE];/* firstname */
	char lastname[DNDS_USER_LASTNAME_SIZE];	/* lastname */
	char email[DNDS_USER_EMAIL_SIZE];	/* email */
	uint8_t role;				/* role */
	uint8_t flag;				/* flags */
	uint32_t age;				/* age */
} __attribute__((__packed__)) DNDS_USER;

enum {
	DNDS_USER_ID = 0,
	DNDS_USER_CONTEXT,
	DNDS_USER_NAME,
	DNDS_USER_PASSWORD,
	DNDS_USER_FIRSTNAME,
	DNDS_USER_LASTNAME,
	DNDS_USER_EMAIL,
	DNDS_USER_ROLE,
	DNDS_USER_FLAG,
	DNDS_USER_AGE,

	END_DNDS_USER
};

static inline void dnds_user_hton(DNDS_USER *obj)
{
	obj->id = htonl(obj->id);
	obj->context = htons(obj->context);
	obj->age = htonl(obj->age);
}

static inline void dnds_user_ntoh(DNDS_USER *obj)
{
	obj->id = ntohl(obj->id);
	obj->context = ntohs(obj->context);
	obj->age = ntohl(obj->age);
}

extern struct dnds *dnds_encapsulate(uint16_t, void *, uint16_t);
extern void dnds_fini();
extern int dnds_init();

#endif /* XIA_DNDS_H */
