/*
 * See COPYRIGHTS file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "journal.h"
#include "xsm.h"

static struct xsm *xsm_encapsulate(uint16_t request_id, void *request, uint16_t request_size)
{
	struct xsm *xsm_request;
	
	xsm_request = malloc(sizeof(struct xsm) + request_size);
	xsm_request->version = XSM_VERSION;
	xsm_request->request_id = request_id;

	memmove(xsm_request + 1, request, request_size);

	return xsm_request;
}

struct xsm *xsm_encapsulate_auth(char *context_name)
{
	struct xsm *xsm_request;
	XSM_AUTH xsm_auth;
	strncpy(xsm_auth.context_name, context_name, XSM_AUTH_CONTEXT_NAME_LEN);
	
	xsm_request = xsm_encapsulate(XSM_REQUEST_AUTH, &xsm_auth, sizeof(XSM_AUTH));

	return xsm_request;
}

struct xsm *xsm_encapsulate_ack(char *ip)
{
	struct xsm *xsm_request;
	XSM_ACK xsm_ack;
	strncpy(xsm_ack.ip, ip, XSM_ACK_IP_LEN);

	xsm_request = xsm_encapsulate(XSM_REQUEST_ACK, &xsm_ack, sizeof(XSM_ACK));

	return xsm_request;
}

struct xsm *xsm_encapsulate_bye(uint8_t reason)
{
	struct xsm *xsm_request;
	XSM_BYE xsm_bye;

	xsm_bye.reason = reason;

	xsm_request = xsm_encapsulate(XSM_REQUEST_BYE, &xsm_bye, sizeof(XSM_BYE));

	return xsm_request;
}
