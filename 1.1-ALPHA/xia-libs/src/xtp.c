/*
 * See COPYRIGHTS file.
 */

#include "journal.h"
#include "netbus.h"
#include "xtp.h"

void xtp_print(void *frame)
{
	size_t i;
	struct xtp *xtp_hdr;
	xtp_hdr = frame;

	journal_notice("xtp]> version %u\n", xtp_hdr->version);
	journal_notice("xtp]> flags   %x\n", xtp_hdr->flag);
	journal_notice("xtp]> len     %u\n", ntohs(xtp_hdr->len));
	journal_notice("xtp]> subchannel %u\n", xtp_hdr->subchannel);
	journal_notice("xtp]> type    %u\n", xtp_hdr->type);

	printf("xtp]> raw { ");
	for (i = 0; i < XTP_HEADER_SIZE + ntohs(xtp_hdr->len); i++) {
		printf("%x ", ((uint8_t*)xtp_hdr)[i]);
	} printf(" } \n");
}

uint16_t xtp_get_len(void *frame)
{
	struct xtp *xtp_hdr;
	xtp_hdr = frame;

	return ntohs(xtp_hdr->len);
}

uint8_t xtp_get_version(void *frame)
{
	struct xtp *xtp_hdr;
	xtp_hdr = frame;

	return xtp_hdr->version;
}

struct xtp *xtp_encapsulate(uint8_t subchannel_id, void *message, uint16_t len, uint8_t type)
{
	struct xtp *xtp_frame;
	xtp_frame = malloc(XTP_HEADER_SIZE + len);

	xtp_frame->version = XTP_VERSION;
	xtp_frame->len = htons(len);
	xtp_frame->flag = 0;
	xtp_frame->subchannel = subchannel_id;
	xtp_frame->type = type;

	memmove((uint8_t*)xtp_frame + XTP_HEADER_SIZE, message, len);

	return xtp_frame;
}

int xtp_output(channel_session, dnds_msg, msg_len)
{

}
 
int xtp_bind_channel(peer_t *peer)
{

}

int xtp_shutdown_channel(int channel_id)
{

}

int xtp_get_subchannel_id()
{

}

int xtp_release_subchannel_id(int subchannel_id)
{

}
