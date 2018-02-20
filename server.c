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
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUFSIZE     80
#define port        3304
#define numOfClient 5
#define numOfServer 3
#define maxNumOfFile 100

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

char     servername[numOfServer][BUFSIZE];
char     clientname[numOfClient][BUFSIZE];
char     filename[maxNumOfFile][BUFSIZE];
char     filenames[BUFSIZ];
char     directoryname[numOfServer][BUFSIZE];
int      serverID;
char*    home_dir;

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
    
    strcpy(servername[0], "dc01.utdallas.edu");
    strcpy(servername[1], "dc02.utdallas.edu");
    strcpy(servername[2], "dc03.utdallas.edu");
    
    strcpy(clientname[0], "dc04.utdallas.edu");
    strcpy(clientname[1], "dc05.utdallas.edu");
    strcpy(clientname[2], "dc06.utdallas.edu");
    strcpy(clientname[3], "dc07.utdallas.edu");
    strcpy(clientname[4], "dc08.utdallas.edu");
    
    strcpy(directoryname[0], "dir1");
    strcpy(directoryname[1], "dir2");
    strcpy(directoryname[2], "dir3");
    
    
    
    DIR *d;
    struct dirent *dir;
    char directory[BUFSIZ];
    
    gethostname(host, BUFSIZE);
    int i = 0;
    for(i = 0; i < 3; ++i){
        if(strcmp(host, servername[i]) == 0){
            serverID = i+1;
            home_dir = directoryname[i];
        }
    }
    
    if (getcwd(directory, sizeof(directory)) != NULL){
        fprintf(stdout, "Current working dir: %s\n", directory);
    }
    else{
        perror("getcwd() error");
        exit(1);
    }
    strcat(directory, "/");
    strcat(directory, home_dir);
    printf("%s\n", directory);
    d = opendir(directory);
    if (d) {
        int i = 0;
        while ((dir = readdir(d)) != NULL) {
            if( dir->d_type != DT_DIR ) {
                strcpy(filename[i], dir->d_name);
                strcat(filenames, dir->d_name);
                strcat(filenames, "|");
                ++i;
            }
        }
        closedir(d);
    }
    
    
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
    
    char path[BUFSIZ];
    strcpy(path, home_dir);
    strcat(path, "/");
    strcat(path, filename[req->file]);
    
    if(req->type == 0){
        FILE *f = fopen(path, "r");
        if (!f) {
            printf("Error loading file.\n");
            printf("path is %s\n", path);
            _exit(1);
        }
        char *line;
        char buf[BUFSIZ];
        size_t len = 0;
        line = (char *)malloc(BUFSIZ * sizeof(char));
        while (getline(&line, &len, f) != -1) {             // read from file line by line
            strcpy(buf, line);
        }
        free(line);
        fclose(f);
        
        char mesg[BUFSIZ];
        sprintf(mesg, "Read last line from %s on server%d: %s", filename[req->file], serverID, buf);
        send_message(sd, mesg);
        printRequest(req);
    }
    else if(req->type == 1){
        
        FILE *f = fopen(path, "a");
        if (!f) {
            printf("Error loading file.\n");
            printf("path is %s\n", path);
            _exit(1);
        }
        fprintf(f, "%s", req->line);
        fclose(f);
        
        char mesg[BUFSIZ];
        sprintf(mesg, "Write to %s on server%d succeed\n", filename[req->file], serverID);
        send_message(sd, mesg);
        printRequest(req);
    }
    else{
        send_message(sd, filenames);
        printRequest(req);
    }
    
    close(sd);
    return 0;
}

void printRequest(Request* req){
    if(req->type == 0)
       printf("Get read request from client%d, target file is %s\n", req->clientID, filename[req->file]);
    else if(req->type == 1)
        printf("Get write request from client%d, target file is %s, append line: %s", req->clientID, filename[req->file], req->line);
    else
        printf("Get list all files request from client%d\n", req->clientID);
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
                
                
