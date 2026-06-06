#include "../include/log.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

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

void log_escribir(const char *accion) {
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

void log_login_escribir(const char *usuario, const char *accion){

    FILE *f = abrirLog();

    time_t ahora = time(NULL);
    struct tm *t  = localtime(&ahora);
    char fecha[32];
    strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", t);

    fprintf(f, "[%s] %s : %s\n", fecha, usuario, accion);
    fflush(f);
    fclose(f);
}

void log_mostrar_desde(time_t desde) {
    FILE *f = fopen("log.txt", "r");
    if (!f) { printf("  (no hay fichero de log)\n"); return; }

    char linea[512];
    int total = 0;
    while (fgets(linea, sizeof(linea), f)) {
        if (linea[0] != '[') continue;

        int anio, mes, dia, hora, min, seg;
        if (sscanf(linea, "[%d-%d-%d %d:%d:%d]",
                   &anio, &mes, &dia, &hora, &min, &seg) != 6) continue;

        struct tm t = {0};
        t.tm_year  = anio - 1900;
        t.tm_mon   = mes - 1;
        t.tm_mday  = dia;
        t.tm_hour  = hora;
        t.tm_min   = min;
        t.tm_sec   = seg;
        t.tm_isdst = -1;

        if (mktime(&t) >= desde) {
            printf("  %s", linea);
            total++;
        }
    }
    fclose(f);
    if (total == 0) printf("  (sin actividad desde el inicio de sesion)\n");
}

void log_seguir_en_tiempo_real() {
    FILE *f = fopen("log.txt", "r");
    if (!f) { printf("  (no hay fichero de log)\n"); return; }

    printf("\n--- LOGS EN TIEMPO REAL (pulsa INTRO para volver) ---\n\n");
    fflush(stdout);

    // Mostrar todo el contenido actual del fichero
    char linea[512];
    while (fgets(linea, sizeof(linea), f)) {
        printf("  %s", linea);
    }
    fflush(stdout);

    // Configurar stdin en modo no bloqueante
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    // Seguir leyendo líneas nuevas hasta que el usuario pulse INTRO
    while (1) {
        // Comprobar si el usuario pulso INTRO
        char c;
        ssize_t n = read(STDIN_FILENO, &c, 1);
        if (n > 0 && (c == '\n' || c == '\r')) {
            break;
        }

        // Leer nuevas líneas del fichero
        if (fgets(linea, sizeof(linea), f)) {
            printf("  %s", linea);
            fflush(stdout);
        } else {
            // No hay líneas nuevas, esperar un poco
            usleep(300000); // 300ms
            clearerr(f);
        }
    }

    // Restaurar stdin bloqueante
    fcntl(STDIN_FILENO, F_SETFL, flags);

    fclose(f);
    printf("\n[INFO] Saliendo de logs en tiempo real.\n");
}