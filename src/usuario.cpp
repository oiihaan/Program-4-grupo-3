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
#include <iostream>

#define MAXDATASIZE 1024

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

extern "C" int usuario_conectar(const char *ip, const char *port)
{
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(ip, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
        printf("Intentando conectarse a %s...\n", s);

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("connect");
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "Error: no se pudo conectar al servidor\n");
        return 2;
    }
    

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("Conectado al servidor en %s:%s\n\n", s, port);

    freeaddrinfo(servinfo);

    while (1) {
        printf("Ingrese comando (o 'exit' para salir): ");
        fflush(stdout);

        char buffer[MAXDATASIZE];
        memset(buffer, 0, MAXDATASIZE);

        if (fgets(buffer, MAXDATASIZE, stdin) == NULL) {
            break;
        }

        buffer[strcspn(buffer, "\n")] = 0;

        if (strcmp(buffer, "exit") == 0) {
            printf("Desconectando...\n");
            break;
        }

        if (send(sockfd, buffer, strlen(buffer), 0) == -1) {
            perror("send");
            break;
        }

        memset(buf, 0, MAXDATASIZE);
        numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0);

        if (numbytes == -1) {
            perror("recv");
            break;
        }

        if (numbytes == 0) {
            printf("Servidor cerró la conexión\n");
            break;
        }

        buf[numbytes] = '\0';
        printf("Respuesta del servidor: %s\n\n", buf);
    }

    close(sockfd);
    return 0;
}
