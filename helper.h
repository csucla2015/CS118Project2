#include <stdio.h>
#include <signal.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/time.h>
#include "packet.h"




void initPacket(struct packet * pkt)
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

/*Taken from Kirby for now, we can figure out how to do our own timer alter */
//signal timeout stuff
volatile sig_atomic_t timeout = 0;


void catch_alarm (int sig) /* signal handler */
{
    timeout = 1;
    signal (sig, catch_alarm);
}

unsigned int setTimeout(unsigned int microseconds) {
    timeout = 0;
    struct itimerval old, current;
    current.it_interval.tv_usec = 0;
    current.it_interval.tv_sec = 0;
    current.it_value.tv_usec = (long int) microseconds; 
    current.it_value.tv_sec = 0;
    if (setitimer (ITIMER_REAL, &current, &old) < 0)
        return 0;
    else
        return old.it_value.tv_sec;
}