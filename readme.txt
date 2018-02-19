First, set working directory to project1 using command: cd project1
Second, compile c files:
   compile server: open three terminals and connect to dc01~dc03 respectively
                   then compile using command: gcc server.c -lpthread -o server
   compile client: open five terminals and connect to dc04~dc08 respectively
                   then compile using command: gcc client.c -lpthread -o client
There are three directories: dir1~dir3, each corresponding to each server. In each diretory, there are three files.
The files in each directory are exactly the same.

Finally, run three servers using command: ./server;
run five clients using command: ./client

Note:
All of the servers and clients will be running in port 3304;
Clients must start at almost the same time since if they fail to connect to each other, they will exit.
The client at the beginning will sleep five seconds to allow time for synchronizaiton.
Once start, they will keep running by randomly generating read or write request to any one of the three files.
To stop execution, use control + c to terminate one client. Then other clients will fail to connect and exit.
Then use control + c to stop server
