/* server_bridge.c */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <iostream>


#define PORT "5555"
#define BACKLOG 5
#define MAXDATASIZE 1024

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) return &(((struct sockaddr_in*)sa)->sin_addr);
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void) {
    int sockfd = -1;
    struct addrinfo hints, *servinfo, *p;
    int rv, yes = 1;
    char addrstr[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; /* FORZAR IPv4 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), addrstr, sizeof addrstr);
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
            perror("setsockopt");
            close(sockfd);
            continue;
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("bind");
            close(sockfd);
            continue;
        }
        break;
    }
    freeaddrinfo(servinfo);
    if (p == NULL) { fprintf(stderr, "Fallo bind\n"); return 2; }
    if (listen(sockfd, BACKLOG) == -1) { perror("listen"); close(sockfd); return 3; }

    printf("Servidor: listo en puerto %s\n", PORT);

    while (1) {
        struct sockaddr_storage addr;
        socklen_t addrlen = sizeof addr;
        int cliente_fd = -1;

        printf("Esperando cliente...\n");
        cliente_fd = accept(sockfd, (struct sockaddr*)&addr, &addrlen);
        if (cliente_fd == -1) {
             perror("accept cliente");
             continue;
        }
        inet_ntop(addr.ss_family, get_in_addr((struct sockaddr*)&addr), addrstr, sizeof addrstr);
        printf("cliente conectado desde %s fd=%d\n", addrstr, cliente_fd);

        char buffer[MAXDATASIZE];
        memset(buffer, 0, MAXDATASIZE);
        int recibido;

        while (1) {
            recibido = recv(cliente_fd, buffer, MAXDATASIZE - 1, 0);
            if (recibido == -1) {
                perror("recv cliente");
                break;
            } else if (recibido == 0) {
                printf("cliente cerró la conexión\n");
                break;
            } else {
                buffer[recibido] = '\0';
                printf("Mensaje recibido: %s\n", buffer);

                if (strcmp(buffer, "exit") == 0) {
                    printf("cliente solicitó cerrar la conexión\n");
                    break;
                }
            }
            memset(buffer, 0, MAXDATASIZE);
        }

        close(cliente_fd);
        printf("Servidor: conexión cerrada\n");
    }

    close(sockfd);
    return 0;
}


