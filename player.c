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

int build_client(const char * master_host_name, const char* port_num){
    int status;
    struct addrinfo hints;
    struct addrinfo *res;
    memset(&hints, 0, sizeof(hints)); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    // printf("master_host_name: %s\n", master_host_name);
    if ((status = getaddrinfo(master_host_name, port_num, &hints, &res)) != 0) {   
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
    }
    int sockfd;
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sockfd == -1){
        fprintf(stderr, "socket error: %s\n", gai_strerror(sockfd));
    }

    if((status = connect(sockfd, res->ai_addr, res->ai_addrlen) == -1)){
        fprintf(stderr, "connect error: %s\n", gai_strerror(status));
    }

    freeaddrinfo(res);
    return sockfd;
}

int get_port_num(int socket_fd) {
  struct sockaddr_in sa;
  socklen_t sa_len = sizeof(sa);
  if (getsockname(socket_fd, (struct sockaddr *)&sa, &sa_len) == -1) {
    fprintf(stderr, "getsockname has some error\n");
  }
  return ntohs(sa.sin_port);
}

int build_server(const char* port_num){
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

    if((status = bind(sockfd, res->ai_addr, res->ai_addrlen)) == -1){
        fprintf(stderr, "bind error: %s\n", gai_strerror(status));
    }
    
    if((status = listen(sockfd, BACKLOG)) == -1){
        fprintf(stderr, "listen error: %s\n", gai_strerror(status));
    }
    freeaddrinfo(res);
    return sockfd;
}

int main(int argc, char * argv[]){
    const char * master_host_name = argv[1];
    const char * player_port = argv[2];
    if(argc != 3){
        fprintf(stderr, "Please provide the correct format of the input\n");
        return 0;
    }
    // printf("master_host_name from main: %s\n", master_host_name);
    int master_fd = build_client(master_host_name, player_port);
    int my_id;
    int num_players;
    recv(master_fd, &my_id, sizeof(my_id), MSG_WAITALL);
    // printf("my_id: %d\n", my_id);
    recv(master_fd, &num_players, sizeof(num_players), MSG_WAITALL);
    printf("Connected as player %d out of %d total players\n", my_id, num_players);
    
    // Send sockfd, port to ringmaster
    int my_sockfd = build_server("");
    // printf("my_sockfd: %d\n", my_sockfd);
    int my_port = get_port_num(my_sockfd);
    int num_bytes;
    if((num_bytes = send(master_fd, &my_port, sizeof(my_port), 0)) == -1){
        fprintf(stderr, "Send port has some error!\n");
    }
    char hostname[200]={'\0'};
    gethostname(hostname, 200);
    // char copy[200]={'\0'};
    // strcpy(copy, master_host_name);
    if((num_bytes = send(master_fd, hostname, sizeof(hostname), 0)) == -1){ //////
            fprintf(stderr, "Send host_name has some error!\n");
    }

    // Receive neighbor_port, neighbor_hostname from ringmaster 
    int neighbor_port;
    char neighbor_hostname[200];
    recv(master_fd, &neighbor_port, sizeof(neighbor_port), 0);
    recv(master_fd, &neighbor_hostname, sizeof(neighbor_hostname), 0);
    // printf("neighbor_hostname: %s\n", neighbor_hostname);
    // Connect with neighbors, and try to be the server and the client
    // server:
    //player work as client, connect to its neighbor's server
    
    char format_neighbor_port[200];
    sprintf(format_neighbor_port, "%d", neighbor_port);
    int right_neighbor_fd = build_client(neighbor_hostname, format_neighbor_port);
    
    //player work as server, accept neighbor's connection
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    addr_size = sizeof(their_addr);
    int left_neighbor_fd;
    if((left_neighbor_fd = accept(my_sockfd, (struct sockaddr *)&their_addr, &addr_size)) == -1){
        fprintf(stderr, "accept error: %s\n", gai_strerror(left_neighbor_fd));
    }

    // start to get potato
    Potato potato;
    fd_set readfds;
    int nfds = left_neighbor_fd > right_neighbor_fd ? left_neighbor_fd : right_neighbor_fd;
    if(master_fd > nfds){
        nfds = master_fd;
    }
    srand((unsigned int)time(NULL) + my_id);
    while(1){
        FD_ZERO(&readfds);
        FD_SET(master_fd, &readfds);
        FD_SET(left_neighbor_fd, &readfds);
        FD_SET(right_neighbor_fd, &readfds);
        select(nfds + 1, &readfds, NULL, NULL, NULL);
        while(1){
            if(FD_ISSET(master_fd, &readfds)){
                recv(master_fd, &potato, sizeof(potato), MSG_WAITALL);
                break;
            }
            if(FD_ISSET(left_neighbor_fd, &readfds)){
                recv(left_neighbor_fd, &potato, sizeof(potato), MSG_WAITALL);
                break;
            }
            if(FD_ISSET(right_neighbor_fd, &readfds)){
                recv(right_neighbor_fd, &potato, sizeof(potato), MSG_WAITALL);
                break;
            }
        }
        int current_hops = potato.hops;
        if(current_hops == 0){
            break;
        } else if(current_hops == 1){
            potato.hops = potato.hops - 1;
            potato.record[potato.counter] = my_id;
            potato.counter = potato.counter + 1;
            printf("I'm it\n");
            int num_bytes = send(master_fd, &potato, sizeof(potato), 0);
        } else{

            potato.hops = potato.hops - 1;
            potato.record[potato.counter] = my_id;
            potato.counter = potato.counter + 1;
            int rand_num = rand() % 2;
            int rand_player_fd = rand_num == 1 ? right_neighbor_fd : left_neighbor_fd;
            if(rand_num == 1){
                printf("Sending potato to %d\n", (my_id + num_players - 1) % num_players);
            } else{
                printf("Sending potato to %d\n", (my_id + num_players + 1) % num_players);
                
            }
            int num_bytes = send(rand_player_fd, &potato, sizeof(potato), 0);
        }
    }
    close(left_neighbor_fd);
    close(right_neighbor_fd);
    close(master_fd);
    return 0;
}