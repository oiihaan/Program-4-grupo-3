#ifndef USUARIO_H
#define USUARIO_H

extern "C" {
int usuario_conectar(const char *ip, const char *port);
int usuario_enviar_recibir(int sockfd, const char *comando, char *respuesta, int max_size);
void usuario_cerrar(int sockfd);
}

#endif