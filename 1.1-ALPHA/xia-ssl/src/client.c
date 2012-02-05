
#include <openssl/ssl.h>

#include <xia/journal.h>
#include <xia/netbus.h>

#include "client.h"
#include "ssl.h"

SSL *ssl;
char buf[512];

void client_auth(peer_t *peer)
{
	int ret;
	printf("client_auth\n");

	ret = SSL_read(ssl, buf, 512);

	printf("ssl read: %i\n", ret);
	printf("ssl err: %s\n", ssl_err_to_string(ssl, ret));
	printf("buf %s\n", buf);

	ssl_error_stack();

	ret = SSL_write(ssl, "PAULPAUL", 10);
	journal_notice("ssl write: %i\n", ret);
}

void client_imux_iface(iface_t *iface)
{
	printf("client_imux_iface\n");
}

void client_imux_peer(peer_t *peer)
{
	printf("client_imux_peer\n");
}

int client_init(peer_t *peer)
{
	SSL_CTX *ctx;
	BIO *sbio;
	BIO *bio_err=0;
	int ret;
	int err;

	char buf[512];

	SSL_library_init(); // Register ciphers
	SSL_load_error_strings(); // Register lib{ssl,crypto} error strings

	ctx = SSL_CTX_new(TLSv1_client_method());

	ret = SSL_CTX_set_cipher_list(ctx, "ADH"); // "ADH-DES-CBC3-SHA");
	journal_notice("set_cipher_list %i\n", ret);

	ret = SSL_CTX_set_tmp_dh(ctx, get_dh1024());
	journal_notice("set_tmp_dh %i\n", ret); 
	SSL_CTX_set_tmp_dh_callback(ctx, tmp_dh_callback);

	// --- certificate 

#define CERTFILE "/home/nib/xia/xia-ssl/cert/client.pem"
#define CAFILE "/home/nib/xia/xia-ssl/cert/rootcert.pem"

	ret = SSL_CTX_use_certificate_chain_file(ctx, CERTFILE);
	journal_notice("certificate chain %i\n", ret);

	ret = SSL_CTX_use_PrivateKey_file(ctx, CERTFILE, SSL_FILETYPE_PEM);
	journal_notice("PrivateKeyfile %i\n", ret);

	ret = SSL_CTX_load_verify_locations(ctx, CAFILE, NULL);
	journal_notice("load_verify_locations %i\n", ret);

	//ret = SSL_CTX_set_default_verify_paths(ctx);
	//journal_notice("verify paths %i\n", ret);	

	//SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_IF_NO_PEER_CERT, NULL);
	SSL_CTX_set_verify_depth(ctx, 1);
	//SSL_CTX_set_options(ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_SINGLE_DH_USE);

	// ----	

	ssl = SSL_new(ctx);

	sbio = BIO_new_socket(peer->socket, BIO_NOCLOSE);
	SSL_set_bio(ssl, sbio, sbio);

	ssl_error_stack();
	while (ret == -1)
	ret = SSL_connect(ssl);
	printf("ssl err: %s\n", ssl_err_to_string(ssl, ret));	
	journal_notice("SSL_connect() %i\n", ret);
	ssl_error_stack();

	ssl_error_stack();
	
	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
	SSL_set_connect_state(ssl);
	printf("hello: %s\n", buf);

	ret = SSL_read(ssl,buf,512);

	printf("ssl err: %s\n", ssl_err_to_string(ssl, ret));	
	printf("read %i\n", ret);
	printf("buf %s\n", buf);


	while (ret == -1)
	ret = SSL_write(ssl, "JOEYJOEY", 10);
	journal_notice("ssl write: %i\n", ret);

	printf("ssl err: %s\n", ssl_err_to_string(ssl, ret));	
	ssl_error_stack();
//	exit(0);
//	peer->send(peer, buf, 10);

	SSL_set_cipher_list(ssl, "RSA");
	SSL_set_verify(ssl, SSL_VERIFY_PEER |
                                        SSL_VERIFY_FAIL_IF_NO_PEER_CERT, 0);


	return 0;
}
