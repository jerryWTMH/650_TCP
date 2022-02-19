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

    if(num_players < 1){
        fprintf(stderr, "number of players is less than 1\n");
    }
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
    int * whole_players;
    whole_players = gamesetting(sockfd, num_players, port_num, whole_players);

    // send information to players to let them know their neighbors
    int Length = sizeof(whole_players) / sizeof(int);
    if(Length > 0){
       for(int i = 0 ; i < Length; i++){
            int right_neighbor = i == (Length - 1)? 0 : (i + 1);
            int left_neighbor = i == 0 ? Length - 1 : (i - 1);
            int array[2] = {right_neighbor, left_neighbor};
            send(sockfd, &array, sizeof(array), 0); //////////////////////////////////At here
       } 
    }
    
    // start game now!
    srand(0);
    int random_player = rand() % num_players;
    Potato potato; 
    potato.hops = num_hops;
    printf("Ready to start the game, sending potato to player %d\n", random_player);
    send(sockfd, & potato, sizeof(potato), 0);
}

int * gamesetting(int sockfd, int num_players, int port_num, int *whole_players){
    for(int i = 0 ; i < num_players; i++){
        struct sockaddr_storage their_addr;
        socklen_t addr_size;
        addr_size = sizeof(their_addr);
        int new_fd;
        if((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)) == -1){
            fprintf(stderr, "accept error: %s\n", gai_strerror(new_fd));
        }
        int player_port;
        int num_bytes = send(new_fd, &i, sizeof(i), 0); // Let the player know its id
        if(num_bytes != sizeof(i)){
            fprintf(stderr, "sned error, the number_bytes is not match: %s\n", gai_strerror(num_bytes));
        }
        recv(new_fd, &player_port, sizeof(player_port), 0);
        if(player_port < 0){
            fprintf(stderr, "player_port received from client is not correct: %s\n", gai_strerror(player_port));
        } else if (player_port == 0){
            fprintf(stderr, "The connection with the client has some problem:  %s\n", gai_strerror(player_port));
        }
        whole_players = realloc(whole_players, (i + 1) * sizeof(*whole_players));
        whole_players[i] = player_port;
        printf("Player %d is ready to play\n", i);
    }
    return whole_players;
}

// the return type of the gamesetting should be modified
