/* 
 * See COPYRIGHTS file
 */

#include <stdio.h>
#include "mbuf.h"

/*
[ ] {pkthdr}	    -nextpkt->	[ ] 	-nextpkt->
|				|
| next in chain			|
V				V
[ ] {data...}			[ ] 
|				|
| next in chain			|
V				V
[ ] {data...}			[ ]
*/

int mbuf_add_in_chain(mbuf_t **mbuf_head, mbuf_t *mbuf_to_add)
{
	if ((*mbuf_head) == NULL)
		return -1;

	if ((*mbuf_head)->prev == NULL) {
		(*mbuf_head)->next = mbuf_to_add;
		(*mbuf_head)->prev = mbuf_to_add;
	}
	else {
		(*mbuf_head)->prev->next = mbuf_to_add;
		(*mbuf_head)->prev = mbuf_to_add;
	}
}

int mbuf_del_in_chain(mbuf_t **mbuf_head, mbuf_t *mbuf_to_del)
{



}

int mbuf_add_packet(mbuf_t **mbuf_head, mbuf_t *mbuf_to_add)
{
	if (*mbuf_head == NULL) {
		*mbuf_head = mbuf_to_add;
		(*mbuf_head)->nextpkt = NULL;
		(*mbuf_head)->prevpkt = mbuf_to_add;
	}
	else {
		mbuf_to_add->nextpkt = NULL;
		mbuf_to_add->prevpkt = (*mbuf_head)->prevpkt;
		(*mbuf_head)->prevpkt->nextpkt = mbuf_to_add;
		(*mbuf_head)->prevpkt = mbuf_to_add;
	}
}

int mbuf_del_packet(mbuf_t **mbuf_head, mbuf_t *mbuf_to_del)
{

	if (*mbuf_head == NULL || mbuf_to_del == NULL)
		return -1;

	if (mbuf_to_del == (*mbuf_head)->prevpkt) {
		(*mbuf_head)->prevpkt = mbuf_to_del->prevpkt;
	}

	if (mbuf_to_del != *mbuf_head) {
		mbuf_to_del->prevpkt->nextpkt = mbuf_to_del->nextpkt;
	}
	
	if (mbuf_to_del->nextpkt != NULL) {
		mbuf_to_del->nextpkt->prevpkt = mbuf_to_del->prevpkt;
	}

	if (mbuf_to_del == *mbuf_head && mbuf_to_del != NULL) {
		*mbuf_head = mbuf_to_del->nextpkt;
	}

	if (mbuf_to_del == *mbuf_head && mbuf_to_del->nextpkt == NULL) {
		*mbuf_head = NULL;
	}

	free(mbuf_to_del->ext_buf);
	free(mbuf_to_del);

	return 0;
}

mbuf_t *mbuf_new(uint8_t *buffer, uint32_t len, uint16_t flags)
{
	mbuf_t *mbuf;
	mbuf = malloc(sizeof(mbuf_t));

	mbuf->ext_buf = malloc(len);
	mbuf->ext_size = len;
	memcpy(mbuf->ext_buf, buffer, len);

	return mbuf;
}

int mbuf_print(mbuf_t **mbuf_head)
{
	mbuf_t *mpkt, *mchain;

	for (mpkt = *mbuf_head; mpkt != NULL; mpkt = mpkt->nextpkt) {
			printf("pkt{%i}||", mpkt->ext_size);
		for (mchain = mpkt->next; mchain != NULL; mchain = mchain->next) {
			printf("chain{%i}||", mchain->ext_size);
		}
		printf("\n");
	}
}

/*
int main()
{
	char buffer[400];

	mbuf_t *mbuf_head = NULL;
	mbuf_t *mbuf_curr;
	mbuf_t *mbuf;



	mbuf = mbuf_new(buffer, 328, MBUF_PKTHDR | MBUF_MORE_FRAGMENT);
	mbuf_add_packet(&mbuf_head, mbuf);

	mbuf = mbuf_new(buffer, 200, MBUF_MORE_FRAGMENT);
	mbuf_add_in_chain(&mbuf_head, mbuf);

	// ----

	mbuf = mbuf_new(buffer, 312, MBUF_PKTHDR | MBUF_MORE_FRAGMENT);
	mbuf_add_packet(&mbuf_head, mbuf);
	mbuf_curr = mbuf;

	mbuf = mbuf_new(buffer, 201, MBUF_MORE_FRAGMENT);
	mbuf_add_in_chain(&mbuf_curr, mbuf);


	mbuf = mbuf_new(buffer, 328, MBUF_PKTHDR);
	mbuf_add_packet(&mbuf_head, mbuf);

	mbuf = mbuf_new(buffer, 350, MBUF_PKTHDR);
	mbuf_add_packet(&mbuf_head, mbuf);

	mbuf = mbuf_new(buffer, 250, MBUF_PKTHDR);
	mbuf_add_packet(&mbuf_head, mbuf);

	mbuf = mbuf_new(buffer, 280, MBUF_PKTHDR);
	mbuf_add_packet(&mbuf_head, mbuf);

	mbuf = mbuf_new(buffer, 210, MBUF_PKTHDR);
	mbuf_add_packet(&mbuf_head, mbuf);


	mbuf_print(&mbuf_head);
}*/
