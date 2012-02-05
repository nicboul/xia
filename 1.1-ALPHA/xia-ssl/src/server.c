
#include <openssl/ssl.h>

#include <xia/journal.h>
#include <xia/netbus.h>

#include "server.h"
#include "ssl.h"

SSL *ssl;
SSL_CTX *ctx;
char buf[50];
STACK_OF(X509_NAME) *cert_name;

static int s_server_session_id_context = 1;
static int s_server_auth_session_id_context = 2;

void server_auth(peer_t *peer)
{
	printf("server auth\n");

	int ret;	
	ret = SSL_read(ssl, buf, 50);
	
	journal_notice("SSL_read %i\n", ret);
	printf("ssl err: %s\n", ssl_err_to_string(ssl, ret));

	printf("read: %s\n", buf);
	
	if (strcmp(buf, "JOEYJOEY") == 0) {

	memset(buf, 0, 50);
		journal_notice("we have to renegotiate!\n");


	// -- certificate

#define CERTFILE "/home/nib/xia/xia-ssl/cert/server.pem"
#define CAFILE "/home/nib/xia/xia-ssl/cert/rootcert.pem"

	ret = SSL_use_certificate_file(ssl, CERTFILE, SSL_FILETYPE_PEM);
	journal_notice("certificate chain %i\n", ret);

	ret = SSL_use_PrivateKey_file(ssl, CERTFILE, SSL_FILETYPE_PEM);
	journal_notice("PrivateKeyfile %i\n", ret);

	cert_name = SSL_load_client_CA_file(CAFILE);
	SSL_set_client_CA_list(ssl, cert_name);

	ret = SSL_CTX_load_verify_locations(ctx, CAFILE, NULL);
	journal_notice("verify loc %i\n", ret);
	// --	

		SSL_set_cipher_list(ssl, "RSA");
		SSL_set_verify(ssl, SSL_VERIFY_PEER |
					SSL_VERIFY_FAIL_IF_NO_PEER_CERT, 0);

		SSL_set_session_id_context(ssl,
		(void*)&s_server_auth_session_id_context,
		sizeof(s_server_auth_session_id_context));

		ret = SSL_renegotiate(ssl);
		journal_notice("renegotiate %i\n");

	printf("ssl err: %s\n", ssl_err_to_string(ssl, ret));
	ssl_error_stack();

		ret = SSL_do_handshake(ssl);
		journal_notice("handshake %i\n");

	printf("ssl err: %s\n", ssl_err_to_string(ssl, ret));
	ssl_error_stack();

		ssl->state = SSL_ST_ACCEPT;

		ret = SSL_do_handshake(ssl);
		journal_notice("handshake %i\n");

	printf("ssl err: %s\n", ssl_err_to_string(ssl, ret));
	ssl_error_stack();


	}

	ssl_error_stack();
}

void server_handshake(peer_t *peer)
{
	printf("server handshake\n");
	
	BIO *sbio;
	int ret;
	
	SSL_library_init();
	SSL_load_error_strings();

	ctx = SSL_CTX_new(TLSv1_server_method());
	SSL_CTX_set_session_id_context(ctx,
	(void*)&s_server_session_id_context,
	sizeof s_server_session_id_context);

	ret = SSL_CTX_set_cipher_list(ctx, "ADH"); //"ADH-DES-CBC3-SHA");
	journal_notice("set_cipher_list %i\n", ret);

	ret = SSL_CTX_set_tmp_dh(ctx, get_dh1024());
	journal_notice("set_tmp_dh %i\n", ret);
	SSL_CTX_set_tmp_dh_callback(ctx, tmp_dh_callback);

	ssl = SSL_new(ctx);

	sbio = BIO_new_socket(peer->socket, BIO_NOCLOSE);
	SSL_set_bio(ssl, sbio, sbio);

	ssl_error_stack();
	while (ret == -1)
	ret = SSL_accept(ssl);

	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);	
	SSL_set_accept_state(ssl);
	journal_notice("SSL_accept() %i\n", ret);
	printf("ssl err: %s\n", ssl_err_to_string(ssl, ret));
	ssl_error_stack();
}

void server_imux_iface(iface_t *iface)
{
	printf("server_imux_iface\n");
}

void server_imux_peer(peer_t *peer)
{
	printf("server_imux_peer\n");
}

void server_disconnect(peer_t *peer)
{
	printf("server_disconnect\n");
}

int server_init()
{
	return 0;
}
