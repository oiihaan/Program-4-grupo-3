#include "../include/funciones.h"
#include "../include/noticias.h"
#include "../include/db.h"
#include "../include/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

/* Funciones de consulta de noticias que acceden directamente a la BD.
   Compiladas para: admin, servidor.  NO para: cliente (usa sockets). */


/* ── Callbacks internos ────────────────────────────────────────── */

static int callback_mostrar_deportes(void *data, int cols, char **valores, char **nombres)
{
    int *contador = (int *)data;
    const char *id     = (valores[0] && valores[0][0]) ? valores[0] : "-";
    const char *titulo = (valores[1] && valores[1][0]) ? valores[1] : "-";
    const char *enlace = (valores[2] && valores[2][0]) ? valores[2] : "-";
    const char *fecha  = (valores[3] && valores[3][0]) ? valores[3] : "-";
    const char *usuario= (valores[4] && valores[4][0]) ? valores[4] : "-";
    (*contador)++;
    printf("  [%s] %-30s | %-10s | Usuario: %s\n", id, titulo, fecha, usuario);
    if (enlace[0] != '-') printf("      Enlace: %s\n", enlace);
    return 0;
}

static int callback_mostrar_politica(void *data, int cols, char **valores, char **nombres)
{
    int *contador = (int *)data;
    const char *id     = (valores[0] && valores[0][0]) ? valores[0] : "-";
    const char *titulo = (valores[1] && valores[1][0]) ? valores[1] : "-";
    const char *enlace = (valores[2] && valores[2][0]) ? valores[2] : "-";
    const char *fecha  = (valores[3] && valores[3][0]) ? valores[3] : "-";
    const char *usuario= (valores[4] && valores[4][0]) ? valores[4] : "-";
    (*contador)++;
    printf("  [%s] %-30s | %-10s | Usuario: %s\n", id, titulo, fecha, usuario);
    if (enlace[0] != '-') printf("      Enlace: %s\n", enlace);
    return 0;
}

static int callback_listar_noticias(void *data, int cols, char **valores, char **nombres)
{
    int *contador = (int *)data;
    const char *id       = (valores[0] && valores[0][0]) ? valores[0] : "-";
    const char *categoria= (valores[1] && valores[1][0]) ? valores[1] : "-";
    const char *titulo   = (valores[2] && valores[2][0]) ? valores[2] : "-";
    const char *fecha    = (valores[3] && valores[3][0]) ? valores[3] : "-";
    const char *usuario  = (valores[4] && valores[4][0]) ? valores[4] : "-";
    (*contador)++;
    printf("  [%s] %-12s | %-30s | %-10s | Usuario: %s\n",
           id, categoria, titulo, fecha, usuario);
    return 0;
}

/* ── verNoticias ───────────────────────────────────────────────── */

void verNoticias()
{
    int opcion;
    do {
        printf("\n--- CONSULTA DE NOTICIAS ---\n");
        printf("1. Deportes\n");
        printf("2. Politica\n");
        printf("3. El tiempo\n");
        printf("4. Listar todas las noticias\n");
        printf("0. Volver al menu gestion de noticias\n");
        printf("Seleccion: ");

        if (scanf("%d", &opcion) != 1) { limpiarBuffer(); opcion = 0; }

        switch (opcion) {
        case 1: mostrarDeportes(); break;
        case 2: mostrarPolitica(); break;
        case 3: mostrarTiempo();   break;
        case 4: noticia_listar();  break;
        case 0: printf("\nVolviendo al menu de gestion de noticias...\n"); break;
        default: printf("\n[!] Opcion invalida. Intenta de nuevo.\n");
        }
    } while (opcion != 0);
}

/* ── mostrarDeportes ───────────────────────────────────────────── */

void mostrarDeportes()
{
    printf("\n--- NOTICIAS DE DEPORTES ---\n");
    log_escribir("Ha consultado las noticias sobre deportes");
    char *err = NULL;
    int total = 0;
    int res = sqlite3_exec(db,
        "SELECT id_publicacion, titulo, enlace, fecha_publicacion, dni_admin "
        "FROM Publicacion WHERE categoria='Deportes' AND estado='ACTIVA';",
        callback_mostrar_deportes, &total, &err);
    if (res != SQLITE_OK) { printf("[ERROR] %s\n", err); sqlite3_free(err); return; }
    if (total == 0) printf("[INFO] No hay publicaciones en la categoria Deportes.\n");
}

/* ── mostrarPolitica ───────────────────────────────────────────── */

void mostrarPolitica()
{
    printf("\n--- NOTICIAS DE POLITICA ---\n");
    log_escribir("Ha consultado las noticias sobre politica");
    char *err = NULL;
    int total = 0;
    int res = sqlite3_exec(db,
        "SELECT id_publicacion, titulo, enlace, fecha_publicacion, dni_admin "
        "FROM Publicacion WHERE categoria='Politica' AND estado='ACTIVA';",
        callback_mostrar_politica, &total, &err);
    if (res != SQLITE_OK) { printf("[ERROR] %s\n", err); sqlite3_free(err); return; }
    if (total == 0) printf("[INFO] No hay publicaciones en la categoria Politica.\n");
}

/* ── noticia_listar ────────────────────────────────────────────── */

void noticia_listar()
{
    printf("\n--- LISTADO DE LAS NOTICIAS ---\n");
    char *err = NULL;
    int total = 0;
    int res = sqlite3_exec(db,
        "SELECT id_publicacion, categoria, titulo, fecha_publicacion, dni_admin "
        "FROM Publicacion;",
        callback_listar_noticias, &total, &err);
    if (res != SQLITE_OK) { printf("[ERROR] %s\n", err); sqlite3_free(err); return; }
    if (total == 0) printf("[INFO] No hay noticias registradas.\n");
    log_escribir("Ha consultado el listado completo de noticias");
}
