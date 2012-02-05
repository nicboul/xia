
#include <xia/journal.h>
#include <xia/netbus.h>

#include "server.h"
#include "client.h"
#include "ssl.h"

char *ssl_err_to_string(SSL *ssl, int err)
{

	switch (SSL_get_error(ssl, err))
	{
		case SSL_ERROR_NONE:
			return "SSL_ERROR_NONE";

		case SSL_ERROR_SSL:
			return "SSL_ERROR_SSL";

		case SSL_ERROR_WANT_READ:
			return "SSL_ERROR_WANT_READ";

		case SSL_ERROR_WANT_WRITE:
			return "SSL_ERROR_WANT_WRITE";

		case SSL_ERROR_WANT_X509_LOOKUP:
			return "SSL_ERROR_WANT_X509_LOOKUP";

		case SSL_ERROR_SYSCALL:
			return "SSL_ERROR_SYSCALL";

		case SSL_ERROR_ZERO_RETURN:
			return "SSL_ERROR_ZERO_RETURN";

		case SSL_ERROR_WANT_CONNECT:
			return "SSL_ERROR_WANT_CONNECT";

		case SSL_ERROR_WANT_ACCEPT:
			return "SSL_ERROR_WANT_ACCEPT";

		default:
			return "Unknown error...";
	}

	return "";
}


void ssl_error_stack()
{
	const char *file;
	int line;
	unsigned long e;

	do {
		e = ERR_get_error_line(&file, &line);
		printf("stack]> %s\n", ERR_error_string(e, NULL));

	} while (e);
}

DH *tmp_dh_callback(SSL *s, int is_export, int keylength)
{

	printf("keyl %i\n", keylength);

}


DH *get_dh1024()
{
        static unsigned char dh1024_p[]={
                0xDE,0xD3,0x80,0xD7,0xE1,0x8E,0x1B,0x5D,0x5C,0x76,0x61,0x79,
                0xCA,0x8E,0xCD,0xAD,0x83,0x49,0x9E,0x0B,0xC0,0x2E,0x67,0x33,
                0x5F,0x58,0x30,0x9C,0x13,0xE2,0x56,0x54,0x1F,0x65,0x16,0x27,
                0xD6,0xF0,0xFD,0x0C,0x62,0xC4,0x4F,0x5E,0xF8,0x76,0x93,0x02,
                0xA3,0x4F,0xDC,0x2F,0x90,0x5D,0x77,0x7E,0xC6,0x22,0xD5,0x60,
                0x48,0xF5,0xFB,0x5D,0x46,0x5D,0xF5,0x97,0x20,0x35,0xA6,0xEE,
                0xC0,0xA0,0x89,0xEE,0xAB,0x22,0x68,0x96,0x8B,0x64,0x69,0xC7,
                0xEB,0x41,0xDF,0x74,0xDF,0x80,0x76,0xCF,0x9B,0x50,0x2F,0x08,
                0x13,0x16,0x0D,0x2E,0x94,0x0F,0xEE,0x29,0xAC,0x92,0x7F,0xA6,
                0x62,0x49,0x41,0x0F,0x54,0x39,0xAD,0x91,0x9A,0x23,0x31,0x7B,
                0xB3,0xC9,0x34,0x13,0xF8,0x36,0x77,0xF3,
                };
        static unsigned char dh1024_g[]={
                0x02,
                };
        DH *dh;

        if ((dh=DH_new()) == NULL) return(NULL);
        dh->p=BN_bin2bn(dh1024_p,sizeof(dh1024_p),NULL);
        dh->g=BN_bin2bn(dh1024_g,sizeof(dh1024_g),NULL);
        if ((dh->p == NULL) || (dh->g == NULL))
                { DH_free(dh); return(NULL); }
        return(dh);
}

int ssl_init(int ssl_mode, char *ssl_addr, int ssl_port)
{
	journal_ftrace(__func__);
	int ret;
	peer_t *peer;

	if (ssl_mode == SSL_SERVER) {
	
		journal_notice("ssl]> server mode\n");

		ret = netbus_newserv(ssl_addr, ssl_port, server_handshake, server_disconnect, server_auth);
		if (ret < 0) {
			journal_notice("ssl]> netbus_newserv failed :: %s:%i\n", __FILE__, __LINE__);
			return -1;
		}

		server_init();
	}
	else if (ssl_mode == SSL_CLIENT) {

		journal_notice("ssl]> client mode\n");

		peer = netbus_newclient(ssl_addr, ssl_port, NULL, client_auth);
		if (peer == NULL) {
			journal_notice("ssl]> netbus_newclient failed :: %s:%i\n", __FILE__, __LINE__);
			return -1;
		}

		client_init(peer);
	}
	else {
		journal_failure(EXIT_ERR, "ssl]> ssl mode `%s' is invalid, {server|client} :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	return 0;
}
