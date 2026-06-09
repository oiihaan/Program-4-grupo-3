#include "test_framework.h"
#include "../include/db.h"
#include <sqlite3.h>
#include <string.h>

static int _contar_filas(const char *tabla) {
    char sql[128];
    snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM %s;", tabla);
    sqlite3_stmt *stmt;
    int n = 0;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    return n;
}
static void setup(void) {
    db_abrir(":memory:");
    FILE *nul = fopen("/dev/null", "w"); FILE *old = stdout; stdout = nul;
    db_crear_tablas();
    stdout = old; fclose(nul);
}
static void teardown(void) { db_cerrar(); }

/* ---- db_abrir / db_cerrar ---- */
static int test_db_abrir_memoria(void) {
    int r; SILENT_CALL(r, db_abrir(":memory:"));
    ASSERT_EQ(1, r); ASSERT_TRUE(db != NULL);
    db_cerrar(); return 1;
}
static int test_db_abrir_ruta_invalida(void) {
    int r; SILENT_CALL(r, db_abrir("/ruta/que/no/existe/ayto.db"));
    db_cerrar(); return 1;
}

/* ---- db_crear_tablas ---- */
static int test_tablas_creadas(void) {
    setup();
    const char *tablas[] = {"Admin","Espacio","Reserva","Publicacion","TipoLicencia","Licencia"};
    int n = sizeof(tablas) / sizeof(tablas[0]);
    for (int i = 0; i < n; i++) {
        char sql[128];
        snprintf(sql, sizeof(sql),
            "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='%s';", tablas[i]);
        sqlite3_stmt *stmt; int existe = 0;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) existe = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
        }
        if (!existe) { teardown(); return 0; }
    }
    teardown(); return 1;
}
static int test_crear_tablas_idempotente(void) {
    setup();
    FILE *nul = fopen("/dev/null","w"); FILE *old = stdout; stdout = nul;
    db_crear_tablas();
    stdout = old; fclose(nul);
    teardown(); return 1;
}

/* ---- Espacio CRUD ---- */
static int test_insertar_espacio(void) {
    setup();
    int r = db_ejecutar("INSERT INTO Espacio (nombre, capacidad, precio_hora, activo) VALUES ('Sala Test', 10, 5.0, 1);");
    ASSERT_EQ(1, r); ASSERT_EQ(1, _contar_filas("Espacio"));
    teardown(); return 1;
}
static int test_insertar_espacio_nombre_duplicado(void) {
    setup();
    db_ejecutar("INSERT INTO Espacio (nombre, capacidad, precio_hora, activo) VALUES ('Sala A', 20, 10.0, 1);");
    int r; SILENT_CALL(r, db_ejecutar("INSERT INTO Espacio (nombre, capacidad, precio_hora, activo) VALUES ('Sala A', 5, 2.0, 1);"));
    ASSERT_EQ(0, r); ASSERT_EQ(1, _contar_filas("Espacio"));
    teardown(); return 1;
}
static int test_eliminar_espacio(void) {
    setup();
    db_ejecutar("INSERT INTO Espacio (nombre, capacidad, precio_hora, activo) VALUES ('Borrame', 5, 1.0, 1);");
    db_ejecutar("DELETE FROM Espacio WHERE nombre='Borrame';");
    ASSERT_EQ(0, _contar_filas("Espacio"));
    teardown(); return 1;
}
static int test_actualizar_espacio(void) {
    setup();
    db_ejecutar("INSERT INTO Espacio (nombre, capacidad, precio_hora, activo) VALUES ('Pista', 4, 8.0, 1);");
    db_ejecutar("UPDATE Espacio SET activo=0 WHERE nombre='Pista';");
    sqlite3_stmt *stmt; int activo = 1;
    if (sqlite3_prepare_v2(db, "SELECT activo FROM Espacio WHERE nombre='Pista';", -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) activo = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    ASSERT_EQ(0, activo);
    teardown(); return 1;
}

/* ---- Reserva FK ---- */
static int test_reserva_espacio_inexistente_falla(void) {
    setup();
    int r; SILENT_CALL(r, db_ejecutar(
        "INSERT INTO Reserva (id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin, num_personas) "
        "VALUES (999, '12345678Z', '2026-06-10', '10:00', '11:00', 2);"));
    ASSERT_EQ(0, r);
    teardown(); return 1;
}
static int test_reserva_valida(void) {
    setup();
    db_ejecutar("INSERT INTO Espacio (id_espacio, nombre, capacidad, precio_hora, activo) VALUES (1, 'Sala OK', 20, 5.0, 1);");
    int r = db_ejecutar(
        "INSERT INTO Reserva (id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin, num_personas) "
        "VALUES (1, '12345678Z', '2026-06-10', '10:00', '11:00', 2);");
    ASSERT_EQ(1, r); ASSERT_EQ(1, _contar_filas("Reserva"));
    teardown(); return 1;
}

/* ---- Admin UNIQUE ---- */
static int test_admin_dni_unico(void) {
    setup();
    db_ejecutar("INSERT INTO Admin (dni, nombre_usuario, password) VALUES ('11111111H', 'admin1', 'hash1');");
    int r; SILENT_CALL(r, db_ejecutar("INSERT INTO Admin (dni, nombre_usuario, password) VALUES ('11111111H', 'admin2', 'hash2');"));
    ASSERT_EQ(0, r); ASSERT_EQ(1, _contar_filas("Admin"));
    teardown(); return 1;
}

int main(void) {
    RUN_TEST(test_db_abrir_memoria,       "Abre correctamente una BD en memoria");
    RUN_TEST(test_db_abrir_ruta_invalida, "Maneja rutas inexistentes sin crash");
    RUN_TEST(test_tablas_creadas,           "Crea las 6 tablas del sistema");
    RUN_TEST(test_crear_tablas_idempotente, "Llamar dos veces no rompe nada (IF NOT EXISTS)");
    RUN_TEST(test_insertar_espacio,                "Inserta un espacio correctamente");
    RUN_TEST(test_insertar_espacio_nombre_duplicado,"Rechaza nombre duplicado (UNIQUE)");
    RUN_TEST(test_eliminar_espacio,                "Elimina un espacio correctamente");
    RUN_TEST(test_actualizar_espacio,              "Actualiza el estado de un espacio");
    RUN_TEST(test_reserva_espacio_inexistente_falla,"Rechaza reserva con espacio inexistente (FK)");
    RUN_TEST(test_reserva_valida,                   "Inserta una reserva valida");
    RUN_TEST(test_admin_dni_unico, "Rechaza DNI duplicado en Admin (PRIMARY KEY)");
    PRINT_RESULTS();
    return _tests_failed ? 1 : 0;
}
