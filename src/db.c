#include "../include/db.h"
#include <stdio.h>

sqlite3 *db = NULL;

int db_abrir(const char *ruta) {
    if (sqlite3_open(ruta, &db) != SQLITE_OK) {
        printf("[ERROR] No se pudo abrir la BD: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    printf("[OK] Base de datos abierta: %s\n", ruta);
    return 1;
}

void db_cerrar() {
    if (db) sqlite3_close(db);
    printf("[OK] Base de datos cerrada.\n");
}

int db_ejecutar(const char *sql) {
    char *err = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
        printf("[ERROR] SQL: %s\n", err);
        sqlite3_free(err);
        return 0;
    }
    return 1;
}

void db_crear_tablas() {
    db_ejecutar(
        "CREATE TABLE IF NOT EXISTS Admin ("
        "dni TEXT PRIMARY KEY,"
        "nombre_usuario TEXT NOT NULL,"
        "password TEXT NOT NULL,"
        "activo INTEGER DEFAULT 1"
        ");"
    );

    db_ejecutar(
        "CREATE TABLE IF NOT EXISTS Espacio ("
        "id_espacio INTEGER PRIMARY KEY AUTOINCREMENT,"
        "nombre TEXT NOT NULL,"
        "capacidad INTEGER,"
        "precio_hora REAL,"
        "activo INTEGER DEFAULT 1"
        ");"
    );

    db_ejecutar(
        "CREATE TABLE IF NOT EXISTS Reserva ("
        "id_reserva INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_espacio INTEGER,"
        "dni_ciudadano TEXT,"
        "fecha TEXT,"
        "franja_inicio TEXT,"
        "franja_fin TEXT,"
        "num_personas INTEGER,"
        "cancelada INTEGER DEFAULT 0,"
        "FOREIGN KEY (id_espacio) REFERENCES Espacio(id_espacio)"
        ");"
    );

    db_ejecutar(
        "CREATE TABLE IF NOT EXISTS Publicacion ("
        "id_publicacion INTEGER PRIMARY KEY AUTOINCREMENT,"
        "tipo TEXT,"
        "categoria TEXT,"
        "titulo TEXT,"
        "enlace TEXT,"
        "dni_admin TEXT,"
        "fecha_publicacion TEXT,"
        "estado TEXT DEFAULT 'ACTIVA',"
        "FOREIGN KEY (dni_admin) REFERENCES Admin(dni)"
        ");"
    );

    db_ejecutar(
        "CREATE TABLE IF NOT EXISTS TipoLicencia ("
        "id_tipo INTEGER PRIMARY KEY AUTOINCREMENT,"
        "nombre TEXT NOT NULL,"
        "descripcion TEXT,"
        "requisitos TEXT,"
        "activo INTEGER DEFAULT 1"
        ");"
    );

    db_ejecutar(
        "CREATE TABLE IF NOT EXISTS Licencia ("
        "id_licencia INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_tipo INTEGER,"
        "dni_ciudadano TEXT,"
        "estado TEXT DEFAULT 'En revision',"
        "fecha_solicitud TEXT,"
        "fecha_expiracion TEXT,"
        "FOREIGN KEY (id_tipo) REFERENCES TipoLicencia(id_tipo)"
        ");"
    );

// FUNCION AUXILIAR PARA HACER PRUEBAS CON LA BD
db_ejecutar(
    "INSERT OR IGNORE INTO Admin (dni, nombre_usuario, password, activo) "
    "VALUES ('12345678A', 'admin', '1234', 1);"
);
    printf("[OK] Tablas creadas/verificadas.\n");
}