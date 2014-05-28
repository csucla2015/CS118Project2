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
#include "packet.c"

#define SERVERPORT "4950"    // the port users will be connecting to

int main(int argc, char *argv[])
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct packet request;
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
    bzero((char *) &request, sizeof(request)); // Change if it causes bugs.
    strcpy(request.data,argv[2]);
    printf("%s",request.data);    

    if ((numbytes = sendto(sockfd, &request, sizeof(request), 0,
             p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }
    
    struct packet response;
    //rspd_pkt.length = DATA_SIZE;
    char* c = "copy_";
    int len = strlen(argv[2]);
    char buf[len+6];
    snprintf(buf, sizeof(buf), "%s%s", c, argv[2]);
    FILE* rec_file = fopen(buf, "w");

    bzero((char *) &response, sizeof(response));
    response.content_len = sizeof(int) * 3;

      if (recvfrom(sockfd, &response, sizeof(response), 0, p->ai_addr, &p->ai_addrlen)) 
            printf("Packet lost!\n");    
    
    fwrite(response.data,1,7,rec_file); 
    freeaddrinfo(servinfo);

    printf("talker: sent %d bytes to %s\n", numbytes, argv[1]);
    close(sockfd);

    return 0;
}
