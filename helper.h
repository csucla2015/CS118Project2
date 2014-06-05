#include <stdio.h>
#include <signal.h>
#include <sys/types.h>   
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <stdlib.h>
#include <strings.h>
#include <sys/time.h>
#include "packet.h"
#include <iostream>





void customBzero(struct packet * pkt)
{
  
  pkt->ack_no=0;
  pkt->seq_no=0;
  pkt->fin =0;
  pkt->size=0;
  pkt->content_len= 0;
  memset(pkt->data,0,1004);
}
int prob( int a ) {
  return ((rand () % 100) < a) ? 1 : 0;
}

