//
//  server.c
//  project1
//
//  Created by Anqi Chen on 2/14/18.
//  Copyright © 2018 Anqi Chen. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>

#define BUFSIZE     80
#define port        3304

typedef struct Request{
    int timestamp;
    int clientID;
    int type;  //type = 0 read; type = 1 write; type = 2 acknowledgment; type = 3 release
    char filename[BUFSIZE];
    char line[BUFSIZE];
}Request;

void* handleClient(void*);
void read_message(int sd, char* mesg);
void send_message(int sd, char* mesg);
void send_request(int sd, Request* req);
void read_request(int sd, Request* req);
void printRequest(Request* req);


int main(int argc, const char * argv[]) {
    
    
    char     host[BUFSIZE];
    int      sd, sd_current;
    int      addrlen;
    int      *sd_client;
    struct   sockaddr_in sin;
    struct   sockaddr_in pin;
    pthread_t tid;
    pthread_attr_t attr;
    
    
    
    /* create an internet domain stream socket */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error creating socket");
        exit(1);
    }
    
    /* complete the socket structure */
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;  /* any address on this host */
    sin.sin_port = htons(port);        /* convert to network byte order */
    
    /* bind the socket to the address and port number */
    if (bind(sd, (struct sockaddr *) &sin, sizeof(sin)) == -1) {
        perror("Error on bind call");
        exit(1);
    }
    
    /* set queuesize of pending connections */
    if (listen(sd, 100) == -1) {
        perror("Error on listen call");
        exit(1);
    }
    
    /* announce server is running */
    gethostname(host, BUFSIZE);
    printf("Server is running on %s:%d\n", host, port);
    
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); /* use detached threads */
    addrlen = sizeof(pin);
    while (1)
    {
        if ((sd_current = accept(sd, (struct sockaddr *)  &pin, (socklen_t*)&addrlen)) == -1)
        {
            perror("Error on accept call");
            exit(1);
        }
        
        sd_client = (int*)(malloc(sizeof(sd_current)));
        *sd_client = sd_current;
        
        pthread_create(&tid, &attr, handleClient, sd_client);
    }
    
    return 0;
}




void* handleClient(void* arg){
    int sd = *((int*)arg);
    free(arg);
    Request* req = (Request*)malloc(sizeof(Request));
    read_request(sd, req);
    printRequest(req);
}

void printRequest(Request* req){
    printf("Get request from %d, Timestamp: %d, request type is %d\n", req->clientID, req->timestamp, req->type);
}


void read_message(int sd, char* mesg){
    int bytes_read = 0;
    int count = 0;
    int len = 0;
    char len_s[2];
    
    // read message size into 2-byte string
    while(bytes_read != 2){
        count = read(sd, len_s + bytes_read, 2 - bytes_read);
        if(count < 0){
            printf("read message size failed\n");
            exit(1);
        }
        bytes_read = bytes_read + count;
    }
    
    // convert string into integer
    len = atoi(len_s);
    
    // read message
    bytes_read = 0;
    count = 0;
    while(bytes_read != len + 1){
        count = read(sd, mesg + bytes_read, len + 1 - bytes_read);
        if(count < 0){
            printf("read message failed\n");
            exit(1);
        }
        bytes_read = bytes_read + count;
    }
}


void send_message(int sd, char* mesg){
    int bytes_sent = 0;
    int count = 0;
    int len = strlen(mesg);
    char len_s[BUFSIZE];
    
    // convert message size into 2-byte string and send
    sprintf(len_s,"%2d",len);
    
    // send message size
    while(bytes_sent != 2){
        count = write(sd, len_s + bytes_sent, 2 - bytes_sent);
        if(count < 0){
            printf("send message size failed\n");
            exit(1);
        }
        bytes_sent = bytes_sent + count;
    }
    
    // send message
    bytes_sent = 0;
    count = 0;
    while(bytes_sent != len + 1){
        count = write(sd, mesg + bytes_sent, len + 1 - bytes_sent);
        if(count < 0){
            printf("send message failed\n");
            exit(1);
        }
        bytes_sent = bytes_sent + count;
    }
}

void read_request(int sd, Request* req){
    int bytes_read = 0;
    int count = 0;
    while(bytes_read != sizeof(Request)){
        count = read(sd, req + bytes_read, sizeof(Request) - bytes_read);
        if(count < 0){
            printf("read char failed\n");
            exit(1);
        }
        bytes_read = bytes_read + count;
    }
}

void send_request(int sd, Request* req){
    int bytes_sent = 0;
    int count = 0;
    while(bytes_sent != sizeof(Request)){
        count = write(sd, req + bytes_sent, sizeof(Request) - bytes_sent);
        if(count < 0){
            printf("send char failed\n");
            exit(1);
        }
        bytes_sent = bytes_sent + count;
    }
}
                
                
