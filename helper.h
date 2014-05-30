#include <stdio.h>
#include <signal.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/time.h>

void initPacket(struct packet * pkt)
{
  
    pkt->ack_no=0;
  pkt->seq_no=0;
  pkt->fin =0;
  pkt->size=0;
  pkt->content_len= 0;
    memset(pkt->data,0,1004);
}