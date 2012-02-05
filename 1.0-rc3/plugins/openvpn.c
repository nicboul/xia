/*
 * OpenVPN plugin to implement the XIA tunnel mode component
 *
 * Copyright (c) 2008; Nicolas Bouliane <nicboul@gmail.com>
 * Copyright (c) 2008; Samuel Jean <peejix@gmail.com>
 * Copyright (c) 2008; Mind4Networks Technologies INC.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include <xia/xiap.h>
#include <xia/session_msg.h>

#include "../lib/netbus.h"

#include <openvpn/openvpn-plugin.h>

static int nil;

static const char *
get_env (const char *name, const char *envp[])
{
	int i;
	const int namelen = strlen(name);
	for (i = 0; envp[i]; ++i)
		if (!strncmp(envp[i], name, namelen)) {
			const char *cp = envp[i] + namelen;
			if (*cp == '=')
				return cp + 1;
		}

	return NULL;
}

OPENVPN_EXPORT openvpn_plugin_handle_t
openvpn_plugin_open_v1 (unsigned int *type_mask,
			const char *argv[],
			const char *envp[])
{
	*type_mask  = OPENVPN_PLUGIN_MASK(OPENVPN_PLUGIN_CLIENT_CONNECT);
	*type_mask |= OPENVPN_PLUGIN_MASK(OPENVPN_PLUGIN_CLIENT_DISCONNECT);

	netbus_init();

	return (openvpn_plugin_handle_t)&nil;
}

OPENVPN_EXPORT int
openvpn_plugin_func_v1 (openvpn_plugin_handle_t handle,
			const int type,
			const char *argv[],
			const char *envp[])
{
	struct session_msg msg, *msg_reply;
	struct xiap x, *x_reply;

	peer_t *peer;
	char mbuf[SLABSIZ];

	const char *name;
	const char *addr, remote[] = "127.0.0.1"; // XXX: hardcoded value
	
	memset(mbuf, 0, SLABSIZ);
	memset(&x, 0, sizeof(struct xiap));
	memset(&msg, 0, sizeof(struct session_msg));
	
	switch (type) {

		case OPENVPN_PLUGIN_CLIENT_CONNECT:
			msg.type = SESSION_MSG_UP;
			break;

		case OPENVPN_PLUGIN_CLIENT_DISCONNECT:
			msg.type = SESSION_MSG_DOWN;
			break;

		default:
			return OPENVPN_PLUGIN_FUNC_ERROR;
	}

	name = get_env("common_name", envp);
	addr = get_env("ifconfig_pool_remote_ip", envp);

	if (name == NULL || addr == NULL)
		return OPENVPN_PLUGIN_FUNC_ERROR;

	peer = netbus_newclient(remote, 9090, NULL, NULL);
	if (peer == NULL)
		return OPENVPN_PLUGIN_FUNC_ERROR;

	strncpy((char *)&msg.name, name, XIAP_NAMESIZ);
	msg.addr.s_addr = inet_addr(addr);
	msg.version = SESSION_MSG_VERSION;

	x.type = XIAMSG_SESSION;
	x.len = htons(sizeof(struct session_msg));
	x.status = XIAP_STATUS_EXEC;
	x.version = XIAP_VERSION;

	memcpy(mbuf + sizeof(struct xiap), &msg, sizeof(struct session_msg));
	memcpy(mbuf, &x, sizeof(struct xiap));

	if (peer->send(peer, mbuf, SLABSIZ) > 0 && peer->recv(peer) > 0) {
		x_reply = (struct xiap *)peer->buf;
		if (x_reply->status != XIAP_STATUS_OK) {
			peer->disconnect(peer);
			return OPENVPN_PLUGIN_FUNC_ERROR;
		}

		++x_reply;
		msg_reply = (struct session_msg *)x_reply;

		if (msg_reply->type == msg.type) {
			peer->disconnect(peer);
			return OPENVPN_PLUGIN_FUNC_SUCCESS;
		} else	{
			// server does not agree, server is right.
			// let him know we understood his wish
			msg.type = msg_reply->type;
			memcpy(mbuf + sizeof(struct xiap), &msg,
			    sizeof(struct session_msg));
			memcpy(mbuf, &x, sizeof(struct xiap));
			peer->send(peer, mbuf, SLABSIZ);
		}
	} 

	peer->disconnect(peer);
	return OPENVPN_PLUGIN_FUNC_ERROR;
}

OPENVPN_EXPORT void
openvpn_plugin_close_v1 (openvpn_plugin_handle_t handle)
{
}

OPENVPN_EXPORT void
openvpn_plugin_abort_v1 (openvpn_plugin_handle_t handle)
{
}
