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
#include <sys/time.h>

#include "helper.h"

#include <vector>


using namespace std;


#define PACKET_SIZE 1024 // change
#define TIMEOUT 5 //
#define MYPORT "5100"    // the port users will be connecting to


#define MAXBUFLEN 100




void catch_alarm (int sig) /* signal handler */
{
    timeout = 1;
    std::cout<<"entered catch_alarm\n";
    signal (sig, catch_alarm);

}



// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



int main(int argc, char *argv[])
{

    // struct timeval tim;  
    // double start_time;
    // double current_time;
   if (argc != 3) {
        fprintf(stderr,"usage: ./server probLoss probCorrupt\n");
        exit(1);
    }
    signal(SIGALRM, catch_alarm);
    int probLoss = atoi(argv[1]);
    int probCorrupt = atoi(argv[2]);
    
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

        // int flags = fcntl(sockfd, F_GETFL);
        

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }


    //fcntl(sockfd, F_SETFL, O_NONBLOCK);

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);
    cout << " Waiting for the client to send us a file" << endl;
    customBzero(&request);

    addr_len = sizeof their_addr;
    cout << addr_len;
    //This takes care of receiving the initial request
    if ((numbytes = recvfrom(sockfd, &request, sizeof(request) , 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    strcpy(fileName, request.data);
    cout << "File name requested is " << fileName << endl;
    //////////////////////////////////////////////////////
    cout << "We go a packet from "<< inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr), s, sizeof s) << endl;
    

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
        customBzero(&p);
        packet_vec.push_back(p);

        p.seq_no = total_sequence+1;

        p.size = fread(p.data, 1, 1004, req_file);
        printf ("Sender: size is %d\n", p.size);

        packet_vec[total_sequence] = p;

        printf ("packet %d size %d\n", total_sequence, packet_vec[total_sequence].size);
        if( nbytes = sendto (sockfd, &packet_vec[total_sequence], PACKET_SIZE, 0,
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
   //We might also need a base variable.
   //According to the demp you star the timer immediately after sending the first packet(that is the timer for the first packet)
      setTimeout(5000);

    // gettimeofday(&tim, NULL);  
    // start_time = tim.tv_sec+(tim.tv_usec/1000000.0); 


   bool stop = false;
   int rec_ack = 0;

   while(1) {
        
       
        if(timeout!=0)
            cout<<"timeout: "<<timeout<<endl;
        struct packet ack;
        customBzero(&ack);
        while(1) {

              struct timeval tv;
              tv.tv_sec = 5;
              tv.tv_usec = 0; //.5 seconds
              if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
                  perror("Error");
              }

            if ( (numbytes = recvfrom(sockfd, &ack, sizeof(ack) , 0,
                (struct sockaddr *)&their_addr, &addr_len)) < 0 ) {
                    //timeout reached
                    cout<<"We reached timeout, we are resending at this point" << endl;
                    int start_index = packet_vec.size() - window_size;

                        for(k = start_index; k < packet_vec.size(); k++) 
                        {
                            printf ("packet %d size %d\n", k, packet_vec[k].size);
                            if( nbytes = sendto (sockfd, &packet_vec[k], PACKET_SIZE, 0,
                                (struct sockaddr *) &their_addr, addr_len) < 0)
                            {
                                if( errno == EWOULDBLOCK ) {
                                    continue;
                                }
                                cout <<"sendto failed2";
                            }

                       }
                       setTimeout(5000);
                       continue;

            }
            else break; //received something
        }

        if (prob(probCorrupt) || prob(probLoss)) 
         {
            fprintf(stderr, "packet was corrupted or lost\n");
            continue;
        }
        else
            printf("Server: ack number %d receieved\n", ack.ack_no);



        struct packet ack1;

        if(ack.ack_no >= rec_ack){
            alarm(0);

            int slide_num = (ack.ack_no - rec_ack);


            for(int i = 0; i < slide_num; i++) 
            {
             
                struct packet p;
                customBzero(&p);
                packet_vec.push_back(p);

                p.seq_no = total_sequence+1;
                p.size = fread(p.data, 1, 1004, req_file);
                printf ("Sender: size is %d\n", p.size);

                packet_vec[total_sequence] = p;

                printf ("packet %d size %d\n", total_sequence, packet_vec[total_sequence].size);
                if( nbytes = sendto (sockfd, &packet_vec[total_sequence], PACKET_SIZE, 0,
                    (struct sockaddr *) &their_addr, addr_len) < 0)
                {
                    if( errno == EWOULDBLOCK ) {
                        continue;
                    }
                    cout <<"sendto failed2";
                }
                total_sequence++;


                if((feof(req_file) || ferror(req_file))) 
                {
                    while(1)
                    {
                         customBzero(&ack1);

                        if(ack.ack_no == (total_sequence-1) )
                        {
                            stop = true;                
                            rec_ack = ack.ack_no;
                            break;
                        }  
                        else
                        {    

                          struct timeval tv;
                          tv.tv_sec = 5;
                          tv.tv_usec = 0; //.5 seconds
                          if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) 
                          {
                              perror("Error");
                          }

                          if ( (numbytes = recvfrom(sockfd, &ack1, sizeof(ack1) , 0, (struct sockaddr *)&their_addr, &addr_len)) < 0 ) {
                            //timeout reached
                            cout<<"We reached timeout, we are resending at this point" << endl;
                            int start_index1 = packet_vec.size() - window_size;

                                for(k = start_index1; k < packet_vec.size(); k++) 
                                {
                                    printf ("packet %d size %d\n", k, packet_vec[k].size);
                                    if( nbytes = sendto (sockfd, &packet_vec[k], PACKET_SIZE, 0,
                                        (struct sockaddr *) &their_addr, addr_len) < 0)
                                    {
                                        if( errno == EWOULDBLOCK ) {
                                            continue;
                                        }
                                        cout <<"sendto failed2";
                                    }

                               }

                              setTimeout(5000);
                               continue;
                             }
                             rec_ack = ack1.ack_no;

                             cout << " Received Ack"<< ack1.ack_no << endl;
                            if(ack1.ack_no == total_sequence -1)
                            {
                                alarm(0);
                                stop = true;
                                break;
                            }
                        }    
                    }    
                                                       
                }
                if(stop) break;

            }



            if(stop==true)
            {  

                cout << "Final received ack is " << rec_ack << endl;
                break;
            } 
            else   
                rec_ack = ack.ack_no;

             

            setTimeout(5000);
                // gettimeofday(&tim, NULL);  
                //   start_time = tim.tv_sec+(tim.tv_usec/1000000.0);  

        }
           
   }

    fclose(req_file);
    struct packet terminate;
    customBzero(&terminate);
    terminate.fin = 1;
    //send the packet
    while(1) 
    {    
        if( nbytes = sendto (sockfd, &terminate, PACKET_SIZE, 0,
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







