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
        // printf("IP_recv: %s\n", ip_recv);
        // printf("amount: %d\n", amount);
        /////////// ip_recv[amount] = '\0'; 
        // for(int i = 0 ; i < 200 ;i ++){
        //     printf("%c",ip_recv[i]); 
        // }
        /////////// if(amount > 200){
        //     fprintf(stderr, "The Length of IP is longer than 200, please adjust the ip length! \n");
        // }

        // char ip[amount + 1];
        // strncpy(ip, &ip_recv[0], amount);
        // ip[amount] = '\0';
        // printf("The IP got from the client: %s \n", ip);
       
        whole_players_fd = (int *)realloc(whole_players_fd, (i + 1) * sizeof(*whole_players_fd));
        whole_players_port = (int *)realloc(whole_players_port, (i + 1) * sizeof(*whole_players_port));
        whole_players_ip = (char **)realloc(whole_players_ip, (i + 1) * sizeof(*whole_players_ip));
        whole_players_fd[i] = new_fd;
        // printf("###############whole_players_fd[%d] : %d\n", i, whole_players_fd[i]);
        whole_players_port[i] = player_port;
        // printf("###############whole_players_port[%d] : %d\n", i, whole_players_port[i]);
        whole_players_ip[i] = ip_recv;
        // printf("###############whole_players_ip[%d] : %s\n", i, whole_players_ip[i]);
        printf("Player %d is ready to play\n", i);
    }
}

void send_neighbor_info_to_players(int num_players, int * whole_players_fd, int * whole_players_port, char ** whole_players_ip){
    int i = 0;
    while(i < num_players){
        int neighbor_id = (i) % num_players;
        // int neighbor_id = (i) % num_players;
        int neighbor_port = whole_players_port[neighbor_id];
        char neighbor_ip[200];
        memset(neighbor_ip, 0, sizeof(neighbor_ip));
        strcpy(neighbor_ip, whole_players_ip[neighbor_id]);
        send(whole_players_fd[(i+1)% num_players], &neighbor_port, sizeof(neighbor_port), 0);
        send(whole_players_fd[(i+1)%num_players], &neighbor_ip, sizeof(neighbor_ip), 0);
        i++;
        
    }
}

void send_to_first_player(Potato * potato, int num_players, int * whole_players_fd){
    srand((unsigned int)time(NULL) + num_players);
    int random_player = rand() % num_players;
    
    printf("Ready to start the game, sending potato to player %d\n", random_player);
    // printf("potato.counter: %d, potato.hops: %d\n", potato->counter, potato->hops);
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
    // printf("nfds: %d\n", nfds);
    FD_ZERO(&readfds);
    for (int i = 0; i < num_players; i++) {
        // Add fd to the set.
        FD_SET(whole_players_fd[i], &readfds);
    }
    select(nfds, &readfds, NULL, NULL, NULL);
    // printf("AFTER SELECT!\n");
    for (int i = 0; i < num_players; i++) {
        // printf("IN THE FOR LOOP[%d] \n",i);
        if (FD_ISSET(whole_players_fd[i], &readfds)) {
            // printf("IN THE IF\n");
            int num_bytes = recv(whole_players_fd[i], potato, sizeof(*potato), 0);
            // printf("num_bytes: %d\n", num_bytes);
            // printf("LAST! potato.counter: %d, potato.hops: %d", potato->counter, potato->hops);
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
    if(num_players < 1){
        fprintf(stderr, "number of players is less than 1\n");
    }
    // Building the server
    int sockfd = build_ringmaster_socket(port_num);

    // Game setting
    int * whole_players_fd = (int *)malloc(sizeof(int));
    int * whole_players_port = (int *)malloc(sizeof(int));
    char ** whole_players_ip = (char **)malloc(sizeof(char));
    game_setting(sockfd, num_players, port_num, whole_players_fd, whole_players_port, whole_players_ip);
    // send information to players to let them know their neighbors
    // printf("%ld\n", sizeof(whole_players_port)/ sizeof(int));
    // for(int i = 0 ; i < sizeof(whole_players_port)/ sizeof(int); i++){
    //     printf("port %d: %d\n", i, whole_players_port[i]);
    // }
    send_neighbor_info_to_players(num_players, whole_players_fd, whole_players_port, whole_players_ip);
    
    // start game now!
    Potato potato; 
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
    
}
// use struct to send the PORT and hostname (or ip address) to player