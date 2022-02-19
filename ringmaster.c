#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include "potato.h"

#define BACKLOG 512
int main(int argc, char * argv[]){
    // input from the commandline
    int port_num = argv[1];
    int num_players = argv[2];
    int num_hops = argv[3];
    // Building the server
    int status;
    struct addrinfo hints;
    struct addrinfo *res;
    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;  
    if ((status = getaddrinfo(NULL, port_num, &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
    }
    int sockfd;
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_flags);
    if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_flags)) == -1){
        fprintf(stderr, "socket error: %s\n", gai_strerror(sockfd));
    }

    if((status = bind(sockfd, res->ai_addr, res->ai_addrlen)) == -1){
        fprintf(stderr, "bind error: %s\n", gai_strerror(status));
    }
    
    if((status = listen(sockfd, BACKLOG)) == -1){
        fprintf(stderr, "listen error: %s\n", gai_strerror(status));
    }

    freeaddrinfo(res);
    gamesetting(sockfd, num_players, port_num);

    Potato potato;
}

void gamesetting(int sockfd, int num_players, int port_num){
    for(int i = 0 ; i < num_players; i++){
        int newplayer;
        struct sockaddr_storage their_addr;
        socklen_t addr_size;
        addr_size = sizeof their_addr;
        int new_fd;
        if((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)) == -1){
            fprintf(stderr, "accept error: %s\n", gai_strerror(new_fd));
        }
    }
}
