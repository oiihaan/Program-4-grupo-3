#ifndef LOG_H
#define LOG_H
#include <stdio.h>
#include <time.h>

FILE* abrirLog();

void log_set_usuario(const char *usuario);
char* log_get_usuario();
void log_escribir(const char *accion);
void log_login_escribir(const char *usuario, const char *accion);
void log_mostrar_desde(time_t desde);

#endif