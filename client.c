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
#include <pthread.h>
#include <semaphore.h>

#define BUFSIZE     80
#define port        3304
#define numOfClient 5
#define numOfServer 3
#define maxNumOfFile 100
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

char     servername[numOfServer][BUFSIZE];
char     clientname[numOfClient][BUFSIZE];
char     Filename[maxNumOfFile][BUFSIZE];
int      timestamp;
int      clientID;
int      numOfFile;
sem_t    mutex_array[maxNumOfFile];
sem_t    mutex_timestamp;
Request_list* req_list[maxNumOfFile];


void* handleClient(void*);
void* createHost(void*);
void* createRequest(void*);
void read_message(int sd, char* mesg);
void send_message(int sd, char* mesg);
void send_request(int sd, Request* req);
void read_request(int sd, Request* req);
void sendToHost(char* hostname, int* timestamp, int* clientID, int* type, int* filename);
void sendToServer(char* hostname, int* type, int* filename, char* line);
void printRequest(Request* req);
void addRequestToList(Request_list* req_list, Request* req);
void printRequestList(Request_list* req_list);
Request_list* createList(void);
Request* getRequest(Request_list* list, int timestamp, int clientID);
void removeRequest(Request* req);

int main(int argc, char *argv[])
{
    
    pthread_t  SendRequest;
    pthread_t  HandleRequest;
    char       host[BUFSIZ];
    
    strcpy(servername[0], "dc01.utdallas.edu");
    strcpy(servername[1], "dc02.utdallas.edu");
    strcpy(servername[2], "dc03.utdallas.edu");
    
    strcpy(clientname[0], "dc04.utdallas.edu");
    strcpy(clientname[1], "dc05.utdallas.edu");
    strcpy(clientname[2], "dc06.utdallas.edu");
    strcpy(clientname[3], "dc07.utdallas.edu");
    strcpy(clientname[4], "dc08.utdallas.edu");
 
    timestamp = 0;
    
    srand(time(0));
    
    //get list of files
    gethostname(host, BUFSIZE);
    int i = 0;
    for(i = 0; i < 5; ++i){
        if(strcmp(host, clientname[i]) == 0){
            clientID = i+1;
        }
    }
    int type = -1;
    i = rand()%numOfServer;
    sendToServer(servername[i], &type, NULL, NULL);
    
    while(numOfFile == 0){
        printf("No file exists!\n");
    }
    
    printf("There are %d files:\n", numOfFile);
    
    for(i = 0; i < numOfFile; ++i){
        printf("%s\n", Filename[i]);
        req_list[i] = createList();
        sem_init(&mutex_array[i], 0, 1);
    }
    sem_init(&mutex_timestamp, 0, 1);
    
    pthread_create(&HandleRequest, NULL, createHost, NULL);
    usleep(5000000);
    pthread_create(&SendRequest, NULL, createRequest, NULL);
    pthread_join(HandleRequest, NULL);
    pthread_join(SendRequest, NULL);
    
    return 0;
}


void* createRequest(void* arg){
    while(1){
        usleep(500000);
        
        Request* p = NULL;
        int i = 0;
        for(i = 0; i < numOfFile; ++i){
            sem_wait(&mutex_array[i]);
            if(req_list[i]->head->next != req_list[i]->tail && req_list[i]->head->next->clientID == clientID && req_list[i]->head->next->ack == numOfClient){
                p = req_list[i]->head->next;
                sem_post(&mutex_array[i]);
                break;
            }
            else{
                sem_post(&mutex_array[i]);
            }
        }
        
        
        if(p!= NULL){
            if(p->type == 0){
                int i = rand()%numOfServer;
                sendToServer(servername[i], &(p->type), &(p->file), p->line);
            }
            else{
                int i = 0;
                for(i=0; i< numOfServer; ++i){
                    sendToServer(servername[i], &(p->type), &(p->file), p->line);
                }
            }
        }
        
        
        if(p != NULL){
            int type = 3;
            //printf("Releasing :");
            //printRequest(p);
            
            Request* temp = (Request*)malloc(sizeof(Request));
            temp->clientID = p->clientID;
            temp->timestamp = p->timestamp;
            temp->file = p->file;
            
            int i;
            for(i = 0; i < numOfClient; ++i){
                sendToHost(clientname[i], &(temp->timestamp), &(temp->clientID), &type, &(temp->file));
            }

            free(temp);
        }
    
        //send read/write request
        /*
        sem_wait(&mutex_timestamp);
        ++timestamp;
        sem_post(&mutex_timestamp);
        */
         
        int type = (rand()%2);
        if(numOfFile == 0){
            printf("No file exists!\n");
            return NULL;
        }
        int filename = (rand()%numOfFile);
    
        Request* req = (Request*)malloc(sizeof(Request));
        
        int tmp = 0;
        sem_wait(&mutex_timestamp);
        tmp = ++timestamp;
        sem_post(&mutex_timestamp);
        
        req->clientID = clientID;
        req->type = type;
        req->file = filename;
        req->ack = 0;
        req->timestamp = tmp;
    
        if(type == 1){
           sprintf(req->line, "Client id: %d, timestamp: %d\n", clientID, req->timestamp);
        }
    
        i = 0;
        for(i = 0; i < numOfFile; ++i){
            if(req->file == i){
                sem_wait(&mutex_array[i]);
                addRequestToList(req_list[i], req);
                req->ack++;
                sem_post(&mutex_array[i]);
            }
        }
        //printf("File: %d, type: %d, Client: %d, timestamp: %d\n",filename, type, clientID, tmp);
        for(i = 0 ; i < numOfClient; ++i){
            if(i+1 != clientID){
                //printf("File: %d, type: %d, Client: %d, timestamp: %d\n",filename, type, clientID, tmp);
                sendToHost(clientname[i], &tmp, &clientID, &type, &filename);
            }
        }
    }
    return NULL;;
}

void* createHost(void* arg){
    
    char     host[BUFSIZE];
    int      sd, sd_current;
    int      addrlen;
    int      *sd_client;
    struct   sockaddr_in sin;
    struct   sockaddr_in pin;
    pthread_attr_t attr;
    pthread_t tid;
    
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
    
    /* announce client is running */
    gethostname(host, BUFSIZE);
    int i = 0;
    for(i = 0; i < numOfClient; ++i){
        if(strcmp(host, clientname[i]) == 0){
            clientID = i+1;
        }
    }
    printf("Client is running on %s:%d\n", host, port);
    
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
        //handleClient(sd_client);
    }
    
    return NULL;;
}


void* handleClient(void* arg){
    int sd = *((int*)arg);
    free(arg);
    Request* req = (Request*)malloc(sizeof(Request));
    read_request(sd, req);
    close(sd);
    
    sem_wait(&mutex_timestamp);
    timestamp = MAX(timestamp, req->timestamp);
    sem_post(&mutex_timestamp);

    if((req->type == 0 || req->type == 1) && (req->clientID != clientID)){
      
        int i = 0;
        for(i = 0; i < numOfFile; ++i){
            if(req->file == i){
                sem_wait(&mutex_array[i]);
                addRequestToList(req_list[i], req);
                sem_post(&mutex_array[i]);
            }
        }
        
        //send ack
        int type = 2;
        sendToHost(clientname[req->clientID-1],&(req->timestamp), &(req->clientID), &type, &(req->file));
        
    }
    if(req->type == 2){
        Request_list* l = NULL;
        int i = 0;
        for(i = 0; i < numOfFile; ++i){
            if(req->file == i){
                l = req_list[i];
                break;
            }
        }
        
        sem_wait(&mutex_array[i]);
        Request* p = getRequest(l, req->timestamp, req->clientID);
        sem_post(&mutex_array[i]);
        
        if(p == NULL){
            printf("NULL error\n");
            printRequest(req);
        }
        else{
            sem_wait(&mutex_array[i]);
            p->ack++;
            sem_post(&mutex_array[i]);
        }
        free(req);
        
    }
    if(req->type == 3){
        //printRequest(req);
        Request_list* l = NULL;
        
        int i = 0;
        for(i = 0; i < numOfFile; ++i){
            if(req->file == i){
                l = req_list[i];
                break;
            }
        }
        
        sem_wait(&mutex_array[i]);
        Request* p = getRequest(l, req->timestamp, req->clientID);
        sem_post(&mutex_array[i]);
        
        if(p == NULL){
           printf("NULL error\n");
           printRequest(req);
        }
        else{
            sem_wait(&mutex_array[i]);
            removeRequest(p);
            sem_post(&mutex_array[i]);
        }
        free(req);
    }
    return NULL;
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
    
    
    //printf("Connecting to %s:%d\n\n", hostname, port);
    
    /* connect to port on host */
    if (connect(sd,(struct sockaddr *)  &pin, sizeof(pin)) == -1) {
        perror("Error on connect call");
        exit(1);
    }
    else{
        Request* req = (Request*)malloc(sizeof(Request));
        req->timestamp = *timestamp;
        req->clientID = *clientID;
        req->type = *type;
        req->file = *filename;
        req->ack = 0;
        
        send_request(sd, req);
        free(req);
        close(sd);
        return;
    }
    
}

void sendToServer(char* hostname, int* type, int* filename, char* line){
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
    
    
    //printf("Connecting to %s:%d\n\n", hostname, port);
    
    /* connect to port on host */
    if (connect(sd,(struct sockaddr *)  &pin, sizeof(pin)) == -1) {
        perror("Error on connect call");
        exit(1);
    }
    else if(*type != -1 ){//write or read
        Request* req = (Request*)malloc(sizeof(Request));
        req->clientID = clientID;
        req->type = *type;
        req->file = *filename;
        strcpy(req->line, line);
        //printf("Line is %s\n", req->line);
        
        char buf[BUFSIZ];
        send_request(sd, req);
        read_message(sd, buf);
        printf("%s", buf);
        free(req);
    }
    else{//list all files
        Request* req = (Request*)malloc(sizeof(Request));
        req->clientID = clientID;
        req->type = *type;
        
        char buf[BUFSIZ];
        send_request(sd, req);
        read_message(sd, buf);

        char *token = strtok(buf, "|");
        int i = 0;
        while (token != NULL)
        {
            strcpy(Filename[i], token);
            token = strtok(NULL, "|");
            ++i;
        }
        numOfFile = i;
        free(req);
    }
    close(sd);
    return;
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

