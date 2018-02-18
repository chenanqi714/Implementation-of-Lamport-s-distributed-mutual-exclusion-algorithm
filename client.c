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
#define numOfClient 5
#define MAX(a,b) (((a)>(b))?(a):(b))

typedef struct Request{
    int timestamp;
    int clientID;
    int type;  //type = 0 read; type = 1 write; type = 2 acknowledgment; type = 3 release
    int file;
    int ack;
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
Request_list* req_list1;
Request_list* req_list2;
Request_list* req_list3;



void handleClient(int* sd);
void read_message(int sd, char* mesg);
void send_message(int sd, char* mesg);
void read_char(int sd, char* c);
void send_char(int sd, char* c);
void send_request(int sd, Request* req);
void read_request(int sd, Request* req);
void sendToHost(char* hostname, int* timestamp, int* clientID, int* type, int* filename);
void printRequest(Request* req);
void addRequestToList(Request_list* req_list, Request* req);
void printRequestList(Request_list* req_list);
Request_list* createList();
Request* getRequest(Request_list* list, int timestamp, int clientID);
Request* executeRequest(int* clientID, Request_list* req_list1, Request_list* req_list2, Request_list* req_list3);
void removeRequest(Request* req);

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
    strcpy(filename[1], "file2");
    strcpy(filename[2], "file3");
    
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
    usleep(5000000);
    timestamp = 0;
    req_list1 = createList();
    req_list2 = createList();
    req_list3 = createList();
    
    addrlen = sizeof(pin);
    while (1)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sd, &readfds);
        struct timeval timeout;
        timeout.tv_sec = 1;
        
        srand(time(NULL));
        
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
            
            printf("List1: ");
            printRequestList(req_list1);
            printf("List2: ");
            printRequestList(req_list2);
            printf("List3: ");
            printRequestList(req_list3);
            
            //send releasing message
            Request* p = executeRequest(&clientID, req_list1, req_list2, req_list3);
            if(p != NULL){
                int type = 3;
                printf("Releasing :");
                printRequest(p);
                sendToHost(clientname[0], &(p->timestamp), &(p->clientID), &type, &(p->file));
                sendToHost(clientname[1], &(p->timestamp), &(p->clientID), &type, &(p->file));
                sendToHost(clientname[2], &(p->timestamp), &(p->clientID), &type, &(p->file));
                sendToHost(clientname[3], &(p->timestamp), &(p->clientID), &type, &(p->file));
                sendToHost(clientname[4], &(p->timestamp), &(p->clientID), &type, &(p->file));
            }
            
            //send read/write request
            ++timestamp;
            int type = (rand()%2);
            int filename = (rand()%3);
            
            Request* req = (Request*)malloc(sizeof(Request));
            req->timestamp = timestamp;
            req->clientID = clientID;
            req->type = type;
            req->file = filename;
            req->ack = 0;
            
            if(req->file == 0){
                addRequestToList(req_list1, req);
            }
            if(req->file == 1){
                addRequestToList(req_list2, req);
            }
            if(req->file == 2){
                addRequestToList(req_list3, req);
            }
            
            req->ack++;
            
            sendToHost(clientname[0], &timestamp, &clientID, &type, &filename);
            sendToHost(clientname[1], &timestamp, &clientID, &type, &filename);
            sendToHost(clientname[2], &timestamp, &clientID, &type, &filename);
            sendToHost(clientname[3], &timestamp, &clientID, &type, &filename);
            sendToHost(clientname[4], &timestamp, &clientID, &type, &filename);
            
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
    if((req->type == 0 || req->type == 1) && (req->clientID != clientID)){
       if(req->file == 0){
           addRequestToList(req_list1, req);
       }
       if(req->file == 1){
           addRequestToList(req_list2, req);
       }
        if(req->file == 2){
           addRequestToList(req_list3, req);
        }

        int type = 2;
        //send ack
        sendToHost(clientname[req->clientID-1],&(req->timestamp), &(req->clientID), &type, &(req->file));
    }
    if(req->type == 2){
        Request_list* l;
        if(req->file == 0){
            l = req_list1;
        }
        else if(req->file == 1){
            l = req_list2;
        }
        else{
            l = req_list3;
        }
        Request* p = getRequest(l, req->timestamp, req->clientID);
        if(p == NULL){
            printf("NULL error\n");
            printRequest(req);
            printf("\n");
        }
        else
            p->ack++;
    }
    if(req->type == 3){
        Request_list* l;
        if(req->file == 0){
            l = req_list1;
        }
        else if(req->file == 1){
            l = req_list2;
        }
        else{
            l = req_list3;
        }
        Request* p = getRequest(l, req->timestamp, req->clientID);
        if(p == NULL){
           printf("NULL error\n");
           printRequest(req);
           printf("\n");
        }
        else{
            printf("Delete: ");
            printRequest(p);
            removeRequest(p);
        }
    }
}

Request_list* createList(){
    Request_list* req_list = (Request_list*)malloc(sizeof(Request_list));
    req_list->head = (Request*)malloc(sizeof(Request));
    req_list->tail = (Request*)malloc(sizeof(Request));
    req_list->head->prev = NULL;
    req_list->head->next = req_list->tail;
    req_list->tail->prev = req_list->head;
    req_list->tail->next = NULL;
    return req_list;
}

Request* getRequest(Request_list* list, int timestamp, int clientID){
    Request* p = list->head->next;
    while(p!=list->tail){
        if(p->clientID == clientID && p->timestamp == timestamp)
            return p;
        else
            p = p->next;
    }
    return NULL;
}

Request* executeRequest(int* clientID, Request_list* req_list1, Request_list* req_list2, Request_list* req_list3){
    Request* p1 = req_list1->head->next;
    Request* p2 = req_list2->head->next;
    Request* p3 = req_list3->head->next;
    if(p1 != req_list1->tail && p1->clientID == *clientID && p1->ack == numOfClient){
        return p1;
    }
    else if(p2 != req_list2->tail && p2->clientID == *clientID && p2->ack == numOfClient){
        return p2;
    }
    else if(p3 != req_list3->tail && p3->clientID == *clientID && p3->ack == numOfClient){
        return p3;
    }
    else{
        return NULL;
    }
}

void removeRequest(Request* req){
    Request* prev = req->prev;
    Request* next = req->next;
    prev->next = next;
    next->prev = prev;
    free(req);
    return;
}

void sendToHost(char *hostname, int* timestamp, int* clientID, int* type, int* filename){
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
        req->timestamp = *timestamp;
        req->clientID = *clientID;
        req->type = *type;
        req->file = *filename;
        req->ack = 0;
        send_request(sd, req);
    }
    
}

void printRequest(Request* req){
    printf("Client %d, Timestamp: %d, request type is %d, request file is %d, ack is %d\n", req->clientID, req->timestamp, req->type, req->file, req->ack);
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
