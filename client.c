//
//  client.c
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
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

#define BUFSIZE     80
#define port        3304
#define numOfClient 2
#define MAX(a,b) (((a)>(b))?(a):(b))

typedef struct Request{
    int timestamp;
    int clientID;
    int type;  //type = 0 read; type = 1 write; type = 2 acknowledgment; type = 3 release
    char filename[BUFSIZE];
    char line[BUFSIZE];
    struct Request* next;
    struct Request* prev;
}Request;

typedef struct Request_list {
    Request* head;
    Request* tail;
}Request_list;

char     servername[3][BUFSIZE];
char     clientname[5][BUFSIZE];
char     filename[3][BUFSIZE];
int      timestamp;
int      clientID;
Request_list* req_list;



void handleClient(int* sd);
void read_message(int sd, char* mesg);
void send_message(int sd, char* mesg);
void read_char(int sd, char* c);
void send_char(int sd, char* c);
void send_request(int sd, Request* req);
void read_request(int sd, Request* req);
void sendToHost(char* hostname, int timestamp);
void printRequest(Request* req);
void addRequestToList(Request_list* req_list, Request* req);
void printRequestList(Request_list* req_list);

int main(int argc, char *argv[])
{
    
    char     host[BUFSIZE];
    int      sd, sd_current;
    int      *sd_client;
    int      addrlen;
    struct   sockaddr_in sin;
    struct   sockaddr_in pin;
    
    
    strcpy(servername[0], "dc01.utdallas.edu");
    strcpy(servername[1], "dc02.utdallas.edu");
    strcpy(servername[2], "dc03.utdallas.edu");
    
    strcpy(clientname[0], "dc04.utdallas.edu");
    strcpy(clientname[1], "dc05.utdallas.edu");
    strcpy(clientname[2], "dc06.utdallas.edu");
    strcpy(clientname[3], "dc07.utdallas.edu");
    strcpy(clientname[4], "dc08.utdallas.edu");
    
    strcpy(filename[0], "file1");
    strcpy(filename[0], "file2");
    strcpy(filename[0], "file3");
    
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
    int i = 0;
    for(i = 0; i < 5; ++i){
        if(strcmp(host, clientname[i]) == 0){
            clientID = i+1;
        }
    }
    printf("Client is running on %s:%d\n", host, port);
    timestamp = 0;
    req_list = (Request_list*)malloc(sizeof(Request_list));
    req_list->head = (Request*)malloc(sizeof(Request));
    req_list->tail = (Request*)malloc(sizeof(Request));
    req_list->head->prev = NULL;
    req_list->head->next = req_list->tail;
    req_list->tail->prev = req_list->head;
    req_list->tail->next = NULL;
    
    addrlen = sizeof(pin);
    while (1)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sd, &readfds);
        struct timeval timeout;
        timeout.tv_sec = 1;
        
        int ret = select(5, &readfds, NULL, NULL, &timeout);
        
        if(ret == -1) //error
            //if ((sd_current = accept(sd, (struct sockaddr *)  &pin, (socklen_t*)&addrlen)) == -1)
        {
            
            perror("Error on select call");
            exit(1);
        }
        else if(ret !=0){ //there exists ready sockets
            sd_current = accept(sd, (struct sockaddr *)  &pin, (socklen_t*)&addrlen);
            sd_client = (int*)(malloc(sizeof(sd_current)));
            *sd_client = sd_current;
            handleClient(sd_client);
            
        }
        else{ // no ready sockets
            ++timestamp;
            sendToHost(servername[0],timestamp);
            sendToHost(clientname[0],timestamp);
            sendToHost(clientname[1],timestamp);
            
            
        }
    }
    return 0;
}


void handleClient(int* arg){
    int sd = *((int*)arg);
    free(arg);
    Request* req = (Request*)malloc(sizeof(Request));
    read_request(sd, req);
    timestamp = MAX(timestamp, req->timestamp);
    addRequestToList(req_list, req);
    printRequestList(req_list);
}

void sendToHost(char *hostname, int timestamp){
    int  sd;
    struct sockaddr_in pin;
    struct hostent *hp;

    
    /* create an Internet domain stream socket */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error creating socket");
        exit(1);
    }
    
    /* lookup host machine information */
    if ((hp = gethostbyname(hostname)) == 0) {
        perror("Error on gethostbyname call");
        exit(1);
    }
    
    /* fill in the socket address structure with host information */
    memset(&pin, 0, sizeof(pin));
    pin.sin_family = AF_INET;
    pin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
    pin.sin_port = htons(port); /* convert to network byte order */
    
    
    printf("Connecting to %s:%d\n\n", hostname, port);
    
    /* connect to port on host */
    if (connect(sd,(struct sockaddr *)  &pin, sizeof(pin)) == -1) {
        perror("Error on connect call");
        //exit(1);
    }
    else{
        Request* req = (Request*)malloc(sizeof(Request));
        req->timestamp = timestamp;
        req->clientID = clientID;
        req->type = 0;
        printf("sending request...");
        send_request(sd, req);
    }
    
}

void printRequest(Request* req){
    printf("Get request from %d, Timestamp: %d, request type is %d\n", req->clientID, req->timestamp, req->type);
}

void addRequestToList(Request_list* req_list, Request* req){
    Request* p = req_list->head->next;
    while(p!=req_list->tail){
        if(p->timestamp < req->timestamp){
            p = p->next;
        }
        else if(p->timestamp == req->timestamp && p->clientID < req->clientID){
            p = p->next;
        }
        else{
            req->next = p;
            req->prev = p->prev;
            p->prev->next = req;
            p->prev = req;
            return;
        }
    }
    req->next = p;
    req->prev = p->prev;
    p->prev->next = req;
    p->prev = req;
    return;
}

void printRequestList(Request_list* req_list){
    Request* p = req_list->head->next;
    while(p!=req_list->tail){
        printRequest(p);
        p = p->next;
    }
    return;
}

void read_char(int sd, char* c){
    int bytes_read = 0;
    int count = 0;
    while(bytes_read != 1){
        count = read(sd, c + bytes_read, 1 - bytes_read);
        if(count < 0){
            printf("read char failed\n");
            exit(1);
        }
        bytes_read = bytes_read + count;
    }
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

void send_char(int sd, char* c){
    int bytes_sent = 0;
    int count = 0;
    while(bytes_sent != 1){
        count = write(sd, c + bytes_sent, 1 - bytes_sent);
        if(count < 0){
            printf("send char failed\n");
            exit(1);
        }
        bytes_sent = bytes_sent + count;
    }
}
