#ifndef PACKET_H
#define PACKET_H

struct packet
{
  int ack_no;
  int seq_no;
  int fin;
  int content_len;
  int size;
  char data[1004];
};

#endif