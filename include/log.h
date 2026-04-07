#ifndef LOG_H
#define LOG_H
#include <stdio.h>


FILE* abrirLog();

void log_set_usuario(const char *usuario);

char* log_get_usuario() ;

void log_escribir( char *accion);

void log_login_escribir(const char *usuario, char *accion);

#endif