/* stubs_db.c - Solo los stubs que necesita test_db (sin db/db_ejecutar) */
#include <stdio.h>
#include <time.h>

void log_escribir(const char *a)               { (void)a; }
void log_set_usuario(const char *u)            { (void)u; }
void log_login_escribir(const char *u, const char *a) { (void)u; (void)a; }
void log_mostrar_desde(time_t d)               { (void)d; }
void log_seguir_en_tiempo_real(void)           {}
char *log_get_usuario(void)                    { return ""; }
FILE *abrirLog(void)                           { return NULL; }
