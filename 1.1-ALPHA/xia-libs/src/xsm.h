/*
 * See COPYRIGHTS file.
 */

#ifndef XIA_XSM_H
#define XIA_XSM_H

#define XSM_VERSION	1

/* 
 * XSM specification
 */
enum {
	XSM_REQUEST_AUTH = 0,
	XSM_REQUEST_ACK,
	XSM_REQUEST_BYE
};

struct xsm {
	uint16_t version;
	uint16_t request_id;
};

static inline void xsm_hton(struct xsm *xsm)
{
	xsm->version = htons(xsm->version);
	xsm->request_id = htons(xsm->request_id);
}

static inline void xsm_ntoh(struct xsm *xsm)
{
	xsm->version = ntohs(xsm->version);
	xsm->request_id = ntohs(xsm->request_id);
}
/* 
 * XSM_AUTH specification
 */
#define XSM_AUTH_CONTEXT_NAME_LEN 64
typedef struct {
	char context_name[XSM_AUTH_CONTEXT_NAME_LEN];
} __attribute__((__packed__)) XSM_AUTH;

/*
 * XSM_ACK specification
 */	
#define XSM_ACK_IP_LEN 15
typedef struct {
	char ip[15];
} __attribute__((__packed__)) XSM_ACK;

/*
 * XSM_BYE specification
 */
enum {
	XSM_BYE_TERMINATE = 0,
	XSM_BYE_AUTH_FAIL
};
typedef struct {
	uint8_t reason;
} __attribute__((__packed__)) XSM_BYE;

extern struct xsm *xsm_encapsulat_auth(char *);
extern struct xsm *xsm_encapsulate_ack(char *);
extern struct xsm *xsm_encapsulate_bye(uint8_t);

#endif /* XIA_XSM_H */
