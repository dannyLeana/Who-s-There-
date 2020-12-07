#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#define BACKLOG 5

// the argument we will pass to the connection-handler threads
struct connection {
    struct sockaddr_storage addr;
    socklen_t addr_len;
    int fd;
};
int readMsgType(char *message);
void echo(struct connection* arg);
int server(char *port);
int currentFD;

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s [port]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
    (void) server(argv[1]);
    return EXIT_SUCCESS;
}

void acceptConnection(struct connection* con, int sfd){
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
    acceptConnection(con, sfd);
    

    // never reach here
    return 0;
}



char* readMessage(int fd){
    int status = -1;
    int retsize = 100;
    char* buffer = malloc(sizeof(char) * retsize);
    buffer[0] = '\0';
    int numread = 0;
	char c;
    int count;
    do{
        status = read(fd, &c, 1);
		if(status == -1) printf("READ ERROR\n");
        numread += status;
        while(numread > retsize){
            retsize *= 2;
            buffer = realloc(buffer, retsize);
        }
        if(status > 0) {
			buffer[numread - 1] = c;
            ioctl(fd, FIONREAD, &count);
        }
    } while(count > 0);
    buffer[numread] = '\0';
    return buffer;
}

void buffered_write(int fd, char *buf, int len) {
	int cur = 0;
	while (cur < len) {
		int nwrite = write(fd, buf + cur, len - cur);
		if (nwrite == -1) {
			fprintf(stderr, "%s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		cur += nwrite;
	}
}

int checkM1(char* m1){
    //returns: 
    //0 if valid message
    //1 if content error
    //2 if length error
    //3 if format error
    //4 if ERR message

    switch(readMsgType(m1)){
        case 0:
            //valid format, continue checking for length and content error
            break;
        case 1:
            //valid error message
            return 4;
        case 2:
            //format error
            return 3;
    }

    //format correct, checking content and length
    const char delim = '|';
    char* type;
    char* contentLen;
    char* content;
    type = strtok(m1, &delim);
    contentLen = strtok(NULL, &delim);
    content = strtok(NULL, &delim);
    //checking content
    if(strcmp("Who's there?", content) != 0){
        return 1;
    }
    //checking length
    if(strlen(content) != atoi(contentLen)){
        return 2;
    }
    //valid message
    return 0;
}

int checkM3(char* m3, char* setup_line){
    //returns: 
    //0 if valid message
    //1 if content error
    //2 if length error
    //3 if format error
    //4 if ERR message

    //checking format
    switch(readMsgType(m3)){
        case 0:
            //valid format, continue checking for length and content error
            break;
        case 1:
            //valid error message
            return 4;
        case 2:
            //format error
            return 3;
    }
    //format correct, checking content and length
    const char delim = '|';
    char* type;
    char* contentLen;
    char* content;
    type = strtok(m3, &delim);
    contentLen = strtok(NULL, &delim);
    content = strtok(NULL, &delim);
    // printf("type:%s\n", type);
    // printf("contentLen:%s\n", contentLen);
    // printf("content:%s\n", content);
    //checking content
    char* setup = malloc(sizeof(char) * strlen(setup_line) + 7);
    strcpy(setup, setup_line);
    strcat(setup, ", who?");
    // printf("setup:%s\n", setup);
    if(strcmp(setup, content) != 0){
        free(setup);
        return 1;
    }
    free(setup);
    //checking length
    if(strlen(content) != atoi(contentLen)){
        return 2;
    }
    //valid message
    return 0;
}

int checkM5(char* m5){
    //returns: 
    //0 if valid message
    //1 if content error
    //2 if length error
    //3 if format error
    //4 if ERR message

    //checking format
    switch(readMsgType(m5)){
        case 0:
            //valid format, continue checking for length and content error
            break;
        case 1:
            //valid error message
            return 4;
        case 2:
            //format error
            return 3;
    }
    //format correct, checking content and length
    const char delim = '|';
    char* type;
    char* contentLen;
    char* content;
    type = strtok(m5, &delim);
    contentLen = strtok(NULL, &delim);
    content = strtok(NULL, &delim);
    // printf("type:%s\n", type);
    // printf("contentLen:%s\n", contentLen);
    // printf("content:%s\n", content);
    
    //checking length
    if(strlen(content) != atoi(contentLen)){
        return 2;
    }
    //checking content
    //content should be a string of letters ended by punctuation
    int i;
    for(i = 0; i < atoi(contentLen) - 1; ++i){
        if(!isalpha(content[i])) return 1;
    }
    if(!ispunct(content[i])) return 1;
    //valid message
    return 0;
}

//close current socket connection and wait for a new connection
void handleError(){
    printf("Exiting session...\n");
    close(currentFD);
}

void sendError(int errorcode, int turn, int fd){
    char* errmsg;
    switch(errorcode){
            case 1:
                //content error
                switch(turn){
                    case 1:
                        errmsg = "ERR|M1CT|";
                        break;
                    case 3:
                        errmsg = "ERR|M3CT|";
                        break;
                    case 5:
                        errmsg = "ERR|M5CT|";
                        break;
                }
                break;
            case 2:
                //length error
                switch(turn){
                    case 1:
                        errmsg = "ERR|M1LN|";
                        break;
                    case 3:
                        errmsg = "ERR|M3LN|";
                        break;
                    case 5:
                        errmsg = "ERR|M5LN|";
                        break;
                }
                break;
            case 3:
                //format error
                switch(turn){
                    case 1:
                        errmsg = "ERR|M1FT|";
                        break;
                    case 3:
                        errmsg = "ERR|M3FT|";
                        break;
                    case 5:
                        errmsg = "ERR|M5FT|";
                        break;
                }
                break;
            case 4:
                handleError();
    }
    printf("sent:\t%s\n", errmsg);
    buffered_write(fd, errmsg, strlen(errmsg));
    handleError();
}

struct joke {
    char* setup;
    char* punchline;
};

void echo(struct connection* arg)
{
    char host[100], port[10];
    struct connection *c = (struct connection *) arg;
    int error;
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

    //create joke
    struct joke curJoke = {"Who", "I didn't know you were an owl!"};
    currentFD = c->fd;
    printf("[%s:%s] connection\n", host, port);
	//EXCHANGING MESSAGES
    int err;
    //send m0
	char* m0 = "REG|13|Knock, knock.|";
    buffered_write(c->fd, m0, strlen(m0));
    printf("sent:\t%s\n", m0);

    //read m1
    char* m1 = readMessage(c->fd);
    printf("read:\t%s\n", m1);
    printf("TYPE CHECK:%d\n",readMsgType(m1));
    //check m1 for errors
    err = checkM1(m1);
    if(err > 0) {
        free(m1);
        sendError(err, 1, c->fd);
        return;
    }
    free(m1);

    //send m2
    char* m2 = "REG|4|Who.|";
    char* setup_line = "Who";
    buffered_write(c->fd, m2, strlen(m2));
    printf("sent:\t%s\n", m2);

    //read m3
    char* m3 = readMessage(c->fd);
    printf("read:\t%s\n", m3);
    //check m3 for errors
    err = checkM3(m3, setup_line);
    if(err > 0) {
        printf("M3 err\n");
        free(m3);
        sendError(err, 3, c->fd);
        return;
    }
    free(m3);

    //send m4
    char* m4 = "REG|30|I didn't know you were an owl!|";
    write(c->fd, m4, strlen(m4));
    printf("sent:\t%s\n", m4);

    //read m5
    char* m5 = readMessage(c->fd);
    printf("read:\t%s\n", m5);
    //check m5 for errors
    err = checkM5(m5);
    if(err > 0) {
        //printf("m5 err\n");
        free(m5);
        sendError(err, 5, c->fd);
        return;
    }
    free(m5);

    close(c->fd);
    free(c);
}
void append(char* s, char c) {
        int len = strlen(s);
        s[len] = c;
        s[len+1] = '\0';
}

int readMsgType(char *message){
    //returns:
    //0 : valid REG
    //1 : valid ERR
    //2 : malformed message
       int pipe_count = 0;
       char buff[strlen(message)];
       char num_buff[256];
       int i=0;
       int j=0;
       int k=0;
       int len;
       int word_check=0;
       int num_check=0;
       int err_check=0;
       //printf("message:%s\n",message);
    if(message[0]=='R'&& message[1]=='E' &&message[2]=='G'){
        i=3;
       while(i<strlen(message)){
           if(message[i]=='|'){
               pipe_count++;
           }
           if(pipe_count==1 && isdigit(message[i])){
               num_check=1;
           }else if(pipe_count==1 && !isdigit(message[i])){
               num_check=0;
           }
           if(pipe_count==2 ){
               if(message[i+1]!='|' && isalpha(message[i+1])){
                   word_check=1;
               }else if(message[i+1]!='|' && isalpha(message[i+1])){
                   word_check=0;
               }
           }
           if(pipe_count==3){
               break;
           }
           i++;
        }
        if(word_check ==1 && num_check ==1 && pipe_count==3){
            return 0; // GOOD
        }
    }else if(message[0]=='E'&& message[1]=='R' && message[2]=='R'){
       
            if(message[3]=='|'){
                err_check+=1;
                if(message[4]=='M'){
                    err_check+=1;
                    if(isdigit(message[5])){
                        err_check+=1;
                        if((message[6]=='C' && message[7]=='T') ||(message[6]=='L' && message[7]=='N')||(message[6]=='F' && message[7]=='T')){
                            err_check+=1;
                        }else{
                            err_check=0;
                        }
                    }else{
                        err_check=0;
                    }
                }else{
                    err_check=0;
                }
            }else{
                err_check=0;
            }
            printf("ERROR CHECK:%d\n",err_check);
        if(err_check==4){
            return 1;
        }
    }

            
    return 2; // BAD
}

    

