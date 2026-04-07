#include "../include/log.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

static char usuario_sesion[64] = "Sistema";


FILE* abrirLog() {
    FILE *f = fopen("log.txt", "a");  // crea el fichero si no existe -->fclose(f); poner luego para cerrar

    if (f == NULL) {
        printf("Error al abrir el fichero log\n");
    }

    return f;
}

void log_set_usuario(const char *usuario) {
    strcpy(usuario_sesion, usuario);
}

char* log_get_usuario() {
    return usuario_sesion;
}

void log_escribir(char *accion) {
    const char *usuario = log_get_usuario();
    FILE *f = abrirLog();
 
    time_t ahora = time(NULL);
    struct tm *t  = localtime(&ahora); // estructura de la libreria de time , digamos qeu es una estructura con todas opciones de fecha
    char fecha[32]; 
    strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", t); //el formater de c 
 
    fprintf(f, "[%s] %s : %s\n", fecha, usuario, accion); //escribe en el fichero
    fflush(f); // fuerza a escribir por si se peta 
    fclose(f);
}

void log_login_escribir(const char *usuario, char *accion){

    FILE *f = abrirLog();
 
    time_t ahora = time(NULL);
    struct tm *t  = localtime(&ahora); // estructura de la libreria de time , digamos qeu es una estructura con todas opciones de fecha
    char fecha[32]; 
    strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", t); //el formater de c 
 
    fprintf(f, "[%s] %s : %s\n", fecha, usuario, accion); //escribe en el fichero
    fflush(f); // fuerza a escribir por si se peta 
    fclose(f);

}

