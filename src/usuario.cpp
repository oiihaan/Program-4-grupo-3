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
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    printf("\n=== INICIANDO CONEXIÓN CON SOCKET ===\n");

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(ip, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        printf("=== ERROR: No se pudo resolver dirección ===\n\n");
        return -1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
        printf("Intentando conectarse a %s:%s...\n", s, port);

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("connect");
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "=== ERROR: No se pudo conectar al servidor ===\n\n");
        freeaddrinfo(servinfo);
        return -1;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("✓ Conectado al servidor en %s:%s\n", s, port);
    printf("=== CONEXIÓN ESTABLECIDA ===\n\n");

    freeaddrinfo(servinfo);

    return sockfd;
}

extern "C" int usuario_enviar_recibir(int sockfd, const char *comando, char *respuesta, int max_size)
{
    if (sockfd == -1) {
        fprintf(stderr, "Error: socket no válido\n");
        return -1;
    }

    if (send(sockfd, comando, strlen(comando), 0) == -1) {
        perror("send");
        return -1;
    }

    int numbytes = recv(sockfd, respuesta, max_size - 1, 0);

    if (numbytes == -1) {
        perror("recv");
        return -1;
    }

    if (numbytes == 0) {
        printf("Servidor cerró la conexión\n");
        return 0;
    }

    respuesta[numbytes] = '\0';
    return numbytes;
}

extern "C" void usuario_cerrar(int sockfd)
{
    if (sockfd != -1) {
        close(sockfd);
        printf("Conexión con servidor cerrada\n");
    }
}
