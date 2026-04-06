#ifndef DB_H
#define DB_H

#include <sqlite3.h>

// Conexión global a la BD
extern sqlite3 *db;

// Funciones principales
int  db_abrir(const char *ruta);
void db_cerrar();
void db_crear_tablas();
int  db_ejecutar(const char *sql);

#endif