/*
 * stubs.c - Implementaciones vacías de dependencias externas
 * para compilar los módulos bajo test sin arrastrar todo el programa.
 */

#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* ---- Base de datos ---- */
sqlite3 *db = NULL;
int db_ejecutar(const char *sql) { (void)sql; return 1; }

/* ---- Log ---- */
void log_escribir(const char *accion)                       { (void)accion; }
void log_set_usuario(const char *usuario)                   { (void)usuario; }
void log_login_escribir(const char *u, const char *a)       { (void)u; (void)a; }
void log_mostrar_desde(time_t desde)                        { (void)desde; }
void log_seguir_en_tiempo_real(void)                        {}
char *log_get_usuario(void)                                 { return ""; }
FILE *abrirLog(void)                                        { return NULL; }

/* ---- Config ---- */
static const char *_apertura = "09:00";
static const char *_cierre   = "21:00";
const char *get_apertura(void) { return _apertura; }
const char *get_cierre(void)   { return _cierre; }
int  definir_intentos(void)    { return 3; }

/* ---- Server ---- */
static int _puerto = 8080;
void server_configurar_desde_puerto(int p) { _puerto = p; }
int  server_get_puerto(void)               { return _puerto; }
void server_set_estado(int e)              { (void)e; }
int  server_get_estado(void)               { return 1; }
void server_verificar_estado(void)         {}
int  server_probar_puerto(int p)           { (void)p; return 1; }

/* ---- Espacios (stub para reservas.c) ---- */
void espacios_listar(void) {}
