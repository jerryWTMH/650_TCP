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

int build_ringmaster_socket(const char* port_num){
    int status;
    struct addrinfo hints;
    struct addrinfo *res;
    memset(&hints, 0, sizeof(hints)); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;  
    if ((status = getaddrinfo(NULL, port_num, &hints, &res)) != 0) {   
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
    }
    int sockfd;
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sockfd == -1){
        fprintf(stderr, "socket error: %s\n", gai_strerror(sockfd));
    }
    int yes = 1;
    status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if((status = bind(sockfd, res->ai_addr, res->ai_addrlen)) == -1){
        fprintf(stderr, "bind error: %s\n", gai_strerror(status));
    }
    
    if((status = listen(sockfd, BACKLOG)) == -1){
        fprintf(stderr, "listen error: %s\n", gai_strerror(status));
    }
    freeaddrinfo(res);
    return sockfd;
}

void game_setting(int sockfd, int num_players, const char * port_num, int *whole_players_fd, int * whole_players_port, char ** whole_players_ip){
    for(int i = 0 ; i < num_players; i++){
        // Prepare to accept client's address from client
        struct sockaddr_storage their_addr;
        socklen_t addr_size;
        addr_size = sizeof(their_addr);
        int new_fd;
        if((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)) == -1){
            fprintf(stderr, "accept error: %s\n", gai_strerror(new_fd));
        }

        int player_port;
        char * ip_recv = (char * )malloc(200);
        int num_bytes = send(new_fd, &i, sizeof(i), 0); // Let the player know its id
        if(num_bytes != sizeof(i)){
            fprintf(stderr, "send error, the number_bytes is not match: %s\n", gai_strerror(num_bytes));
        }
        // set the total number of the players
        if((num_bytes = send(new_fd, &num_players, sizeof(num_players), 0) == -1)){
            fprintf(stderr, "send error, the number_bytes is not match: %s\n", gai_strerror(num_bytes));
        }

        // THE FIRST RECV for PORT
        recv(new_fd, &player_port, sizeof(player_port), 0);
        // printf("player_port: %d", player_port);
         if(player_port < 0){
            fprintf(stderr, "player_port received from client is not correct: %s\n", gai_strerror(player_port));
        } else if (player_port == 0){
            fprintf(stderr, "The connection with the client has some problem:  %s\n", gai_strerror(player_port));
        }

        // THE SECOND RECV for IP
        int amount; // collect the hostname(or ip address) from player
        amount = recv(new_fd, ip_recv, 200, MSG_WAITALL);
       
        whole_players_fd = (int *)realloc(whole_players_fd, (i + 1) * sizeof(*whole_players_fd));
        whole_players_port = (int *)realloc(whole_players_port, (i + 1) * sizeof(*whole_players_port));
        whole_players_ip = (char **)realloc(whole_players_ip, (i + 1) * sizeof(*whole_players_ip));
        whole_players_fd[i] = new_fd;
        whole_players_port[i] = player_port;
        // printf("whole_players_port[%d]: %d\n",i, whole_players_port[i]);
        whole_players_ip[i] = ip_recv;
        printf("Player %d is ready to play\n", i);
    }
}

void send_neighbor_info_to_players(int num_players, int * whole_players_fd, int * whole_players_port, char ** whole_players_ip){
    int i = 0;
    // for(int i = 0; i < num_players; i++){
    //     printf("whole_players_port[%d]: %d\n", i , whole_players_port[i]);
    // }
    while(i < num_players){
        int neighbor_id = (i ) % num_players;
        // int neighbor_id = (i) % num_players;
        
        int neighbor_port = whole_players_port[neighbor_id];
        char neighbor_ip[200];
        memset(neighbor_ip, 0, sizeof(neighbor_ip));
        // printf("check initialize: %s\n", whole_players_ip[neighbor_id]);
        // char * neighbor_ip = strdup(whole_players_ip[neighbor_id]);
        strcpy(neighbor_ip, whole_players_ip[neighbor_id]);
        send(whole_players_fd[(i+1)% num_players], &neighbor_port, sizeof(neighbor_port), 0);
        send(whole_players_fd[(i+1)%num_players], &neighbor_ip, sizeof(neighbor_ip), 0);
        // send(whole_players_fd[(i+1)%num_players], neighbor_ip, sizeof(*neighbor_ip), 0);
        i++;
        
    }
}

void send_to_first_player(Potato * potato, int num_players, int * whole_players_fd){
    srand((unsigned int)time(NULL) + num_players);
    int random_player = rand() % num_players;
    
    printf("Ready to start the game, sending potato to player %d\n", random_player);
    // printf("potato.counter: %d, potato.hops: %d\n", potato->counter, potato->hops);
    printf("%d\n", whole_players_fd[random_player]);
    // printf("sizeof potato: %ld\n", sizeof(*potato));
    send(whole_players_fd[random_player], potato, sizeof(*potato), 0);
}

void recv_potato_from_last(Potato * potato, int num_players, int * whole_players_fd){
    // printf("original potato, potato.counter: %d, potato.hops: %d\n", potato->counter, potato->hops);
    fd_set readfds;
    int nfds = 0; // max fd + 1
    for(int i = 0; i < num_players; i++){
        if(whole_players_fd[i] > nfds){
            nfds = whole_players_fd[i];
        }
    }
    nfds += 1;
    FD_ZERO(&readfds);
    for (int i = 0; i < num_players; i++) {
        // Add fd to the set.
        FD_SET(whole_players_fd[i], &readfds);
    }
    select(nfds, &readfds, NULL, NULL, NULL);
    for (int i = 0; i < num_players; i++) {
        if (FD_ISSET(whole_players_fd[i], &readfds)) {
            int num_bytes = recv(whole_players_fd[i], potato, sizeof(*potato), 0);
            break;
        }
    }
}

void print_trace_of_potato(Potato potato){
    printf("Trace of potato: \n");
    for (int i = 0; i < potato.counter; i++) {
        if(i != 0){
            printf(",");
        }
        printf("%d", potato.record[i]);
        if(i == potato.counter - 1){
            printf("\n");
        }
    }
}
int main(int argc, char * argv[]){
    // input from the commandline
    const char * port_num = argv[1];
    int num_players = atoi(argv[2]);
    int num_hops = atoi(argv[3]);
    if(num_players <= 1){
        return 0;
        // fprintf(stderr, "number of players is less than 1\n");
    }
    printf("Potato Ringmaster\n");
    printf("Players = %d\n", num_players);
    printf("Hops = %d\n", num_hops);
    // Building the server
    int sockfd = build_ringmaster_socket(port_num);

    // Game setting
    int * whole_players_fd = (int *)malloc(sizeof(int));
    int * whole_players_port = (int *)malloc(sizeof(int));
    char ** whole_players_ip = (char **)malloc(sizeof(char *));
    //
    for(int i = 0 ; i < num_players; i++){
        // Prepare to accept client's address from client
        struct sockaddr_storage their_addr;
        socklen_t addr_size;
        addr_size = sizeof(their_addr);
        int new_fd;
        if((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)) == -1){
            fprintf(stderr, "accept error: %s\n", gai_strerror(new_fd));
        }

        int player_port;
        char * ip_recv = (char * )malloc(200);
        int num_bytes = send(new_fd, &i, sizeof(i), 0); // Let the player know its id
        if(num_bytes != sizeof(i)){
            fprintf(stderr, "send error, the number_bytes is not match: %s\n", gai_strerror(num_bytes));
        }
        // set the total number of the players
        if((num_bytes = send(new_fd, &num_players, sizeof(num_players), 0) == -1)){
            fprintf(stderr, "send error, the number_bytes is not match: %s\n", gai_strerror(num_bytes));
        }

        // THE FIRST RECV for PORT
        recv(new_fd, &player_port, sizeof(player_port), 0);
        // printf("player_port: %d", player_port);
         if(player_port < 0){
            fprintf(stderr, "player_port received from client is not correct: %s\n", gai_strerror(player_port));
        } else if (player_port == 0){
            fprintf(stderr, "The connection with the client has some problem:  %s\n", gai_strerror(player_port));
        }

        // THE SECOND RECV for IP
        int amount; // collect the hostname(or ip address) from player
        amount = recv(new_fd, ip_recv, 200, MSG_WAITALL);
        if(i >= 1){
            whole_players_fd = (int *)realloc(whole_players_fd, (i + 1) * sizeof(*whole_players_fd));
            whole_players_port = (int *)realloc(whole_players_port, (i + 1) * sizeof(*whole_players_port));
            whole_players_ip = (char **)realloc(whole_players_ip, (i + 1) * sizeof(*whole_players_ip));
        }
        whole_players_fd[i] = new_fd;
        whole_players_port[i] = player_port;
        // printf("whole_players_port[%d]: %d\n",i, whole_players_port[i]);
        whole_players_ip[i] = ip_recv;
        printf("Player %d is ready to play\n", i);
    }
    //
    // game_setting(sockfd, num_players, port_num, whole_players_fd, whole_players_port, whole_players_ip);
    send_neighbor_info_to_players(num_players, whole_players_fd, whole_players_port, whole_players_ip);
    
    // start game now!
    
    Potato potato; 
    memset(&potato, 0, sizeof(potato));
    potato.hops = num_hops;
    potato.counter = 0;
    
    if(num_hops > 0){
        // send the potato to the first random player!
        send_to_first_player(&potato, num_players, whole_players_fd);   
        // recv the final result 
        recv_potato_from_last(&potato, num_players, whole_players_fd);   
        // send potato with num_hops 0 to all players to shut down
        for (int i = 0; i < num_players; i++) {
            Potato potato_zero;
            memset(&potato_zero, 0, sizeof(potato_zero));
            potato_zero.hops = 0;
            send(whole_players_fd[i], &potato_zero, sizeof(potato_zero), 0);
        }
        // print the trace 
        print_trace_of_potato(potato);       
        // closing the whole file descriptor
        for (int i = 0; i < num_players; i++) {
            close(whole_players_fd[i]);
        }
        close(sockfd);
        return 0;
    }
    free(whole_players_fd);
    free(whole_players_ip);
    for(int i = 0 ; i < num_players; i++){
        free(whole_players_ip[i]);
    }
    free(whole_players_ip);
    
}