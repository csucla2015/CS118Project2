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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <time.h>
#include "packet.h"
#include "helper.h"

#include <vector>


using namespace std;


#define DATAGRAM_SIZE 1024 // change
#define TIMEOUT 10000 //
#define MYPORT "5100"    // the port users will be connecting to


#define MAXBUFLEN 100

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    char fileName[1004];  //Change 
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;

    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    struct packet request;
    struct packet response;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    int window_size = 5;
    struct packet packets[window_size]; // change

    vector<packet> packet_vec;
    //packet_vec.resize(5);


    int nbytes = 0;

    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    printf("listener: waiting to recvfrom...\n");
    initPacket(&request);

    addr_len = sizeof their_addr;
    cout << addr_len;
    //This takes care of receiving the initial request
    if ((numbytes = recvfrom(sockfd, &request, sizeof(request) , 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }
        printf("listener: waiting to recvfrom1...\n");
    strcpy(fileName, request.data);
    cout << fileName;
    //////////////////////////////////////////////////////
    printf("listener: got packet from %s\n",
    inet_ntop(their_addr.ss_family,
        get_in_addr((struct sockaddr *)&their_addr),
        s, sizeof s));
    printf("listener: packet is %d bytes long\n", numbytes);
    buf[numbytes] = '\0';
    printf("listener: packet contains \"%s\"\n", buf);

    FILE* req_file = fopen(fileName,"rb");
    if (req_file == NULL) // open failed 
    {
        cout << "failed open file";
    }
    /////////////////////////////////////////////
    //Send initial window_size packets
    //////////////////////////////////////////////
    int base = 0;
    int packetsSent =0;

    base = 1;
    int total_sequence = 0;
    int k;
    for(k = 0; k < window_size; k++) 
    {
        // get the next chunk of the file
        struct packet p;
        initPacket(&p);
        packet_vec.push_back(p);

        p.seq_no = total_sequence+1;

        p.size = fread(p.data, 1, 1004, req_file);
        printf ("Sender: size is %d\n", p.size);

        packet_vec[total_sequence] = p;

        printf ("packet %d size %d\n", total_sequence, packet_vec[total_sequence].size);
        if( nbytes = sendto (sockfd, &packet_vec[total_sequence], DATAGRAM_SIZE, 0,
            (struct sockaddr *) &their_addr, addr_len) < 0)
        {
            if( errno == EWOULDBLOCK ) {
                continue;
            }
            cout <<"sendto failed2";
        }
        total_sequence++;

        if(feof(req_file) || ferror(req_file) ) 
        {
            //done reading file
            break;
        }
   }

   bool stop = false;

   while(1) {

        struct packet ack;
        initPacket(&ack);

        while(1){
            if ((numbytes = recvfrom(sockfd, &ack, sizeof(ack) , 0,
                (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                perror("recvfrom");
                exit(1);
            }
            printf("Server: ack number %d receieved\n", ack.ack_no);

            if(ack.ack_no == total_sequence){
                break; //all are received
            }
        }
        
        for(int i = 0; i < window_size; i ++) {
            struct packet p;
            initPacket(&p);
            packet_vec.push_back(p);

            p.seq_no = total_sequence+1;
            p.size = fread(p.data, 1, 1004, req_file);
            printf ("Sender: size is %d\n", p.size);

            packet_vec[total_sequence] = p;

            printf ("packet %d size %d\n", total_sequence, packet_vec[total_sequence].size);
            if( nbytes = sendto (sockfd, &packet_vec[total_sequence], DATAGRAM_SIZE, 0,
                (struct sockaddr *) &their_addr, addr_len) < 0)
            {
                if( errno == EWOULDBLOCK ) {
                    continue;
                }
                cout <<"sendto failed2";
            }
            total_sequence++;


            if(feof(req_file) || ferror(req_file) ) 
            {
                //done reading file
                stop = true;
                break;
            }
        }

        if(stop) break;

   }

    fclose(req_file);
    struct packet terminate;
    initPacket(&terminate);
    terminate.fin = 1;
    //send the packet
    while(1) 
    {    
        if( nbytes = sendto (sockfd, &terminate, DATAGRAM_SIZE, 0,
               (struct sockaddr *) &their_addr, addr_len) < 0)
        {
            if( errno == EWOULDBLOCK ) 
            {
                continue;
            }
            cout << "sendto failed1";
        }
        break;
    }
      
           

    close(sockfd); 
        
    return 0;
}











