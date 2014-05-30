struct packet
{
  int ack_no;
  int fin_no;
  int fin;
  char* timestamp;
  int content_len;
  char data[128];
};
