#include "../include/auth.h"
#include "../include/db.h"
#include "../include/config.h"
#include "../include/funciones.h"
#include "../include/log.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sha256.h"

#include "../include/auth.h"
#include "../include/db.h"
#include "../include/config.h"
#include "../include/funciones.h"
#include "../include/log.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sha256.h"

char dni_admin_sesion[32] = "";

void auth_generar_hash(const char *password, const char *fecha, char *out_hash) {
    struct tc_sha256_state_struct s;
    uint8_t digest[32];
    char combinado[256];

    snprintf(combinado, sizeof(combinado), "%s%s", password, fecha);

    tc_sha256_init(&s);
    tc_sha256_update(&s, (const uint8_t *)combinado, strlen(combinado));
    tc_sha256_final(digest, &s);

    for (int i = 0; i < 32; i++) {
        sprintf(out_hash + (i * 2), "%02x", digest[i]);
    }
    out_hash[64] = '\0';
}

void admin_registrar_nuevo() {
    char dni[32], usuario[64], password_plano[64];
    char hash_final[65];
    char fecha_aux[32] = "";

    printf("\n--- REGISTRO DE NUEVO ADMINISTRADOR ---\n");
    do {
        printf("DNI: "); scanf("%31s", dni); limpiarBuffer();
    } while (!dni_es_valido(dni));
    printf("Usuario: "); scanf("%63s", usuario); limpiarBuffer();
    printf("Contraseña: "); scanf("%63s", password_plano); limpiarBuffer();

    sqlite3_stmt *stmt;
    // 1: Insertar registro base
    const char *sql_ins = "INSERT INTO Admin (dni, nombre_usuario, password) VALUES (?, ?, 'temp');";
    
    if (sqlite3_prepare_v2(db, sql_ins, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, dni, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, usuario, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            printf("[ERROR] El DNI o Usuario ya existen.\n");
            sqlite3_finalize(stmt);
            return;
        }
        sqlite3_finalize(stmt);
    }

    // 2: Recuperar el timestamp generado por SQLite
    const char *sql_sel = "SELECT fecha_creacion FROM Admin WHERE dni = ?;";
    if (sqlite3_prepare_v2(db, sql_sel, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, dni, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            strcpy(fecha_aux, (const char*)sqlite3_column_text(stmt, 0));
        }
        sqlite3_finalize(stmt);
    }

    if (strlen(fecha_aux) > 0) {
        // 3: Generar Hash y Actualizar
        auth_generar_hash(password_plano, fecha_aux, hash_final);

        const char *sql_upd = "UPDATE Admin SET password = ? WHERE dni = ?;";
        if (sqlite3_prepare_v2(db, sql_upd, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, hash_final, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, dni, -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            printf("[OK] Administrador creado exitosamente con seguridad SHA-256.\n");
            char msg[200];
            snprintf(msg, sizeof(msg), "Se ha creado un  administrador con DNI %s", dni);
            log_escribir(msg);
        }
    }
}

int auth_login() {
    char usuario[64];
    char *password;
    int intentos = definir_intentos();
    printf("\n\n");
    printf("=========================================\n");
    printf(">>> INICIO DE SESION DEL AYUNTAMIENTO <<<\n");
    printf("=========================================\n");

    while (intentos > 0) {
        printf("\nUsuario: ");
        scanf("%63s", usuario);
        limpiarBuffer();
        password = capturar_contrasena();

        if (password == NULL) {
            printf("[ERROR] La contraseña no puede estar vacía.\n");
            continue;
        }

        // 1. Modificamos la consulta: Obtenemos el hash y la fecha para ese usuario
        sqlite3_stmt *stmt;
        const char *sql =
            "SELECT password, fecha_creacion FROM Admin "
            "WHERE nombre_usuario=? AND activo=1;";

        int autenticado = 0;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, usuario, -1, SQLITE_STATIC);
            
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                // Recuperamos el hash almacenado y la sal (fecha)
                const char *hash_db = (const char *)sqlite3_column_text(stmt, 0);
                const char *fecha_db = (const char *)sqlite3_column_text(stmt, 1);

                // 2. Generamos el hash de la contraseña introducida usando la fecha de la DB
                char hash_calculado[65];
                auth_generar_hash(password, fecha_db, hash_calculado);

                // 3. Comparamos los hashes
                if (strcmp(hash_db, hash_calculado) == 0) {
                    autenticado = 1;
                }
            }
            sqlite3_finalize(stmt);
        }

        log_escribir("Ha buscado en la base de datos");
        free(password);

        if (autenticado) {
            printf("\n[OK] Bienvenido, %s!\n", usuario);
            log_set_usuario(usuario);
            log_escribir("Inicio de sesion exitoso");

            // Guardar el DNI del admin que ha iniciado sesion
            const char *sql_dni = "SELECT dni FROM Admin WHERE nombre_usuario=? AND activo=1;";
            if (sqlite3_prepare_v2(db, sql_dni, -1, &stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, usuario, -1, SQLITE_STATIC);
                if (sqlite3_step(stmt) == SQLITE_ROW)
                    strncpy(dni_admin_sesion, (const char*)sqlite3_column_text(stmt, 0), sizeof(dni_admin_sesion) - 1);
                sqlite3_finalize(stmt);
            }
            return 1;
        } else {
            intentos--;
            if (intentos > 0)
                printf("[ERROR] Credenciales incorrectas. Intentos restantes: %d\n", intentos);
            char msg[47];
            snprintf(msg, sizeof(msg), "Login fallido - intentos restantes: %d", intentos);
            log_login_escribir(usuario, msg);
        }
    }

    printf("\n[ERROR] Demasiados intentos fallidos. Cerrando programa.\n");
    log_escribir("Acceso bloqueado por exceso de intentos fallidos");
    return 0;
}

void auth_editar_password(const char* dni) {
    char nueva_pass[64], fecha_aux[32], nuevo_hash[65];
    sqlite3_stmt *stmt;

    printf("Introduce la nueva contraseña: ");
    scanf("%63s", nueva_pass);
    limpiarBuffer();

    // 1. Obtenemos la fecha original
    const char *sql_sel = "SELECT fecha_creacion FROM Admin WHERE dni = ?;";
    if (sqlite3_prepare_v2(db, sql_sel, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, dni, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            strcpy(fecha_aux, (const char*)sqlite3_column_text(stmt, 0));
        }
        sqlite3_finalize(stmt);
    }

    // 2. Generamos el nuevo hash con la misma fecha
    auth_generar_hash(nueva_pass, fecha_aux, nuevo_hash);

    // 3. Guardamos el nuevo hash
    const char *sql_upd = "UPDATE Admin SET password = ? WHERE dni = ?;";
    if (sqlite3_prepare_v2(db, sql_upd, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, nuevo_hash, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, dni, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        printf("[OK] Contraseña actualizada con hash SHA-256.\n");
        char msg[200];
    snprintf(msg, sizeof(msg), "Ha cambiado la contraseña del administrador con DNI %s", dni);
    log_escribir(msg);
    }
}