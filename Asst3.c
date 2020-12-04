#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>
#define BACKLOG 5


// the argument we will pass to the connection-handler threads
struct connection {
    struct sockaddr_storage addr;
    socklen_t addr_len;
    int fd;
};
void readMsgType(char *message);
void echo(struct connection* arg);
int server(char *port);

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s [port]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
    (void) server(argv[1]);
    return EXIT_SUCCESS;
}


int server(char *port)
{
    struct addrinfo hint, *address_list, *addr;
    struct connection *con;
    int error, sfd;
    // initialize hints
    memset(&hint, 0, sizeof(struct addrinfo));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE;
    	// setting AI_PASSIVE means that we want to create a listening socket

    // get socket and address info for listening port
    // - for a listening socket, give NULL as the host name (because the socket is on
    //   the local host)
    error = getaddrinfo(NULL, port, &hint, &address_list);
    if (error != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        return -1;
    }
    // attempt to create socket
    for (addr = address_list; addr != NULL; addr = addr->ai_next) {
        sfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        // if we couldn't create the socket, try the next method
        if (sfd == -1) {
            continue;
        }
        // if we were able to create the socket, try to set it up for
        // incoming connections;
        // 
        // note that this requires two steps:
        // - bind associates the socket with the specified port on the local host
        // - listen sets up a queue for incoming connections and allows us to use accept
        if ((bind(sfd, addr->ai_addr, addr->ai_addrlen) == 0) &&
            (listen(sfd, BACKLOG) == 0)) {
            break;
        }
        // unable to set it up, so try the next method
        close(sfd);
    }
    if (addr == NULL) {
        // we reached the end of result without successfuly binding a socket
        fprintf(stderr, "Could not bind\n");
        return -1;
    }
    freeaddrinfo(address_list);
    // at this point sfd is bound and listening
    printf("Waiting for connection\n");
    for (;;) {
    	// create argument struct for child thread
		con = malloc(sizeof(struct connection));
        con->addr_len = sizeof(struct sockaddr_storage);
        	// addr_len is a read/write parameter to accept
        	// we set the initial value, saying how much space is available
        	// after the call to accept, this field will contain the actual address length
        // wait for an incoming connection
        con->fd = accept(sfd, (struct sockaddr *) &con->addr, &con->addr_len);
        	// we provide
        	// sfd - the listening socket
        	// &con->addr - a location to write the address of the remote host
        	// &con->addr_len - a location to write the length of the address
        	//
        	// accept will block until a remote host tries to connect
        	// it returns a new socket that can be used to communicate with the remote
        	// host, and writes the address of the remote hist into the provided location
        // if we got back -1, it means something went wrong
        if (con->fd == -1) {
            perror("accept");
            continue;
        }
		
		echo(con);
    }

    // never reach here
    return 0;
}

char* readMessage(int fd){
    int status = -1;
    char buffer[256];
    int retsize = 64;
    char* ret = malloc(sizeof(char) * retsize);
    int numread = 0;
    do{
        status = read(fd, buffer, 255);
        numread += status;
        while(numread > retsize){
            retsize *= 2;
            ret = realloc(ret, retsize);
        }
        if(status > 0) {
            buffer[status] = '\0';
            strcat(ret, buffer);
        }
    }while(status > 0);
    return ret;
}

void echo(struct connection* arg)
{
    char host[100], port[10], buf[101];
    struct connection *c = (struct connection *) arg;
    int error, nread;

	// find out the name and port of the remote host
    error = getnameinfo((struct sockaddr *) &c->addr, c->addr_len, host, 100, port, 10, NI_NUMERICSERV);
    	// we provide:
    	// the address and its length
    	// a buffer to write the host name, and its length
    	// a buffer to write the port (as a string), and its length
    	// flags, in this case saying that we want the port as a number, not a service name
    if (error != 0) {
        fprintf(stderr, "getnameinfo: %s", gai_strerror(error));
        close(c->fd);
        return;
    }

    printf("[%s:%s] connection\n", host, port);
	//EXCHANGING MESSAGES
    char buff[256];
	char* m1 = "REG|13|Knock, knock.|";
    printf("sent:\t%s\n", m1);
    write(c->fd, m1, strlen(m1));
    char* m2 = readMessage(c->fd);
    printf("read:\t%s\n", m2);
    //printf("%s\n", m2);

    close(c->fd);
    free(c);
}
void append(char* s, char c) {
        int len = strlen(s);
        s[len] = c;
        s[len+1] = '\0';
}

void readMsgType(char *message){
       int pipe_count = 0;
       char buff[strlen(message)];
       char num_buff[256];
       int i=0;
       int j=0;
       int k=0;
       int len;
       int wordCount=0;
       printf("message:%s\n",message);
        if(message[0]=='R'&& message[1]=='E' &&message[2]=='G'){
                    i=3;
        }else{
            puts("ERROR");
        }
       while(i<strlen(message)){
           if(message[i]=='|'){
               pipe_count++;
           }
           if(pipe_count==1 && isdigit(message[i])){
               num_buff[j]=message[i];
               j++;
           }
           if(pipe_count==2 ){
               if(message[i+1]!='|'){
                buff[k]=message[i+1];
                k++;
                wordCount++;
               }
           }
           if(pipe_count==3){
               break;
           }
           i++;
        }
        num_buff[j] = '\0';
        buff[k]='\0';
        len=atoi(num_buff);
        
       if(len!=wordCount){
            puts("ERROR");
        }else if(pipe_count!=3){
            puts("ERROR");
       }
        else{
            printf("Word:%s\n",buff);
        }
}
        
       



    

