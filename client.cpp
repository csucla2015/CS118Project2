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

#define SERVERPORT "5140"    // the port users will be connecting to

int main(int argc, char *argv[])
{

    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    packet request;
    if (argc != 5) {
        fprintf(stderr,"usage: talker hostname filename probLoss probCorr\n");
        exit(1);
    }
    int probLoss = atoi(argv[3]);
    int probCorr = atoi(argv[4]);
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

    printf("Requesting file %s from server\n",request.data);    

    if ((numbytes = sendto(sockfd, &request, 1024, 0,
             p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }
    
    //variable for receiving response
    //rspd_pkt.length = DATA_SIZE;


    //creating a file name with copy_ appended to the front
    char* c = "copy_";
    int len = strlen(argv[2]);
    char buf[len+6];
    snprintf(buf, sizeof(buf), "%s%s", c, argv[2]);
    FILE* rec_file = fopen(buf, "wb");

        
    //fprintf (stderr, "waiting for message \n");
    struct packet response;
    customBzero(&request);
    customBzero(&response);
    response.ack_no = 1;
    int cumAck = 0;    
    int nbytes = 0;
    bool flag1;
    int total_sequence = 0;
    while(1) {
        flag1 =false;
        //wait for a packet
        if( nbytes = recvfrom(sockfd, &request, 1024, 0, 
                               p->ai_addr, &p->ai_addrlen) >= 0)
        {



            if (request.fin == 1)
            {
                //terminate connection
                 cout << "FIN received seq# " << request.seq_no << ", FIN 1, Content-Length " << request.size  << endl;

                fclose(rec_file);
                break;
            } 


            if (prob(probCorr)) 
             {

                 cout << "Packet Corrupted: Expecting Sequence number " << ((total_sequence)*1004)+request.size << endl;
                 struct packet ack;
                 customBzero(&ack);
                 ack.ack_no = total_sequence;
                  if ((numbytes = sendto(sockfd, &ack, 1024, 0, p->ai_addr, p->ai_addrlen)) == -1) {
                    perror("talker: sendto");
                    exit(1);
                }
                flag1=true;
                continue;
            }

            if(prob(probLoss)){
              cout <<  "Packet Loss: Expecting Sequence number " <<((total_sequence)*1004)+request.size << endl;
              flag1=true;

              struct timeval tv;
              tv.tv_sec = 0;
              tv.tv_usec = 500; //.5 seconds
              if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
                  perror("Error");
              }
              if( nbytes = recvfrom(sockfd, &request, 1024, 0, 
                               p->ai_addr, &p->ai_addrlen) < 0) {

                cerr<<"Timeout reached; resending ACK# "<<total_sequence<<endl;
                struct packet ack;
                 customBzero(&ack);
                 ack.ack_no = total_sequence;
                  if ((numbytes = sendto(sockfd, &ack, 1024, 0, p->ai_addr, p->ai_addrlen)) == -1) {
                    perror("talker: sendto");
                    exit(1);
                }


                continue;

              }

            }

          //  if(request.fin != 1 && flag1 == false) 

            if( request.seq_no != (total_sequence+1))
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
                 cout << "DATA received seq# " << ((total_sequence)*1004)+request.size << ", FIN 0, Content-Length " << request.size  << endl;
                 fwrite(request.data,1,request.size,rec_file);  
                 struct packet ack;
                 customBzero(&ack);
                 ack.ack_no = request.seq_no;
                 total_sequence++;
                  if ((numbytes = sendto(sockfd, &ack, 1024, 0, p->ai_addr, p->ai_addrlen)) == -1) {
                    perror("talker: sendto");
                    exit(1);
                }
                cout << "DATA sent ack# " <<  ((ack.ack_no-1)*1004) + request.size << ", FIN 0, Content-Length 0"  << endl;

        
            }
        }
        else {

           // cout << "recvfrom failed";
        }
        

    }   



      struct packet finack;
     customBzero(&finack);
     finack.ack_no = request.seq_no;
     finack.fin = 1;


    while(1) {
          cout << "FINACK sent ack# " <<  finack.ack_no << ", FIN 1, Content-Length 0"  << endl;

          if ((numbytes = sendto(sockfd, &finack, 1024, 0, p->ai_addr, p->ai_addrlen)) == -1) {
            perror("talker: sendto");
            exit(1);
         }

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 500;

         if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
                  perror("Error");
              }

          struct packet ack_of_finack;
         customBzero(&ack_of_finack);
          if( nbytes = recvfrom(sockfd, &ack_of_finack, 1024, 0, 
                               p->ai_addr, &p->ai_addrlen) < 0) {
            cout<<"Timeout reached waiting for ACK of FINACK; resending"<<endl;
            continue;

          }
          else {
            //ack_of_finack received
            
            if(ack_of_finack.fin == 0){
              cout << "ACK of FINACK received ACK# " << ack_of_finack.ack_no << ", FIN 0, Content-Length " << ack_of_finack.size  << endl;
              break;
            }
            else continue;
          }
    }





    freeaddrinfo(servinfo);
    close(sockfd);

    return 0;
}

