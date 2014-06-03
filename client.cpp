#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <time.h>
#include "packet.h"
#include "helper.h"
using namespace std;


#include <sys/stat.h>

#define SERVERPORT "5100"    // the port users will be connecting to

int main(int argc, char *argv[])
{

    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    packet request;
    if (argc != 3) {
        fprintf(stderr,"usage: talker hostname filename\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    
    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to bind socket\n");
        return 2;
    }

       

    customBzero(&request);
    strncpy(request.data,argv[2],1004);

    printf("%s",request.data);    

    if ((numbytes = sendto(sockfd, &request, 1024, 0,

             p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }
    
    //variable for receiving response
    packet response;
    //rspd_pkt.length = DATA_SIZE;


    //creating a file name with copy_ appended to the front
    char* c = "copy_";
    int len = strlen(argv[2]);
    char buf[len+6];
    snprintf(buf, sizeof(buf), "%s%s", c, argv[2]);
    FILE* rec_file = fopen(buf, "wb");

        
    fprintf (stderr, "waiting for message \n");
    struct packet request;
    struct packet response;
    customBzero(&request);
    customBzero(&response);
    response.ack_no = 1;
    int cumAck = 0;    
    int nbytes = 0;

    int total_sequence = 0;
    while(1) {

        //wait for a packet
        if( nbytes = recvfrom(sockfd, &request, 1024, 0, 
                               p->ai_addr, &p->ai_addrlen) < 0)
        {
            cout << "recvfrom failed";
        }
        else if (request.fin == 1)
        {
            //terminate connection
            
            fclose(rec_file);
            break;
        } 
        else if( request.seq_no != (total_sequence+1))
        {   
             struct packet ack;
             customBzero(&ack);
             ack.ack_no = total_sequence;
              if ((numbytes = sendto(sockfd, &ack, 1024, 0, p->ai_addr, p->ai_addrlen)) == -1) {
                perror("talker: sendto");
                exit(1);
            }

        }
        else  
        {
             fwrite(request.data,1,request.size,rec_file);  
             struct packet ack;
             customBzero(&ack);
             ack.ack_no = request.seq_no;
             total_sequence++;
              if ((numbytes = sendto(sockfd, &ack, 1024, 0, p->ai_addr, p->ai_addrlen)) == -1) {
                perror("talker: sendto");
                exit(1);
            }
    
          }
    }   


    freeaddrinfo(servinfo);
    close(sockfd);

    return 0;
}

