#include "../include/licencias.h"
#include "../include/db.h"
#include "../include/log.h"
#include "../include/funciones.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

// ─── CALLBACKS ────────────────────────────────────────────────────────────────

static int callback_listar_licencias(void *data, int cols, char **valores, char **nombres) {
    int *contador = (int *)data;
    const char *id      = valores[0] ? valores[0] : "-";
    const char *tipo    = valores[1] ? valores[1] : "-";
    const char *dni     = valores[2] ? valores[2] : "-";
    const char *estado  = valores[3] ? valores[3] : "-";
    const char *f_solic = valores[4] ? valores[4] : "-";
    const char *f_expir = valores[5] ? valores[5] : "-";

    (*contador)++;
    printf("  [%s] %-20s | DNI: %-12s | %-12s | Solicitud: %-10s | Expira: %-10s\n",
        id, tipo, dni, estado, f_solic, f_expir);
    return 0;
}

static int callback_listar_tipos(void *data, int cols, char **valores, char **nombres) {
    int *contador = (int *)data;
    const char *id     = valores[0] ? valores[0] : "-";
    const char *nombre = valores[1] ? valores[1] : "-";
    const char *desc   = valores[2] ? valores[2] : "-";
    const char *req    = valores[3] ? valores[3] : "-";
    const char *activo = valores[4] ? valores[4] : "0";

    (*contador)++;
    printf("  [%s] %-20s | %-30s | Requisitos: %-20s | %s\n",
        id, nombre, desc, req, atoi(activo) ? "ACTIVO" : "BAJA");
    return 0;
}

static int callback_existe(void *data, int cols, char **valores, char **nombres) {
    int *existe = (int *)data;
    if (valores[0] && atoi(valores[0]) > 0)
        *existe = 1;
    return 0;
}

static int dni_es_valido(const char *dni) {
    static const char *letras = "TRWAGMYFPDXBNJZSQVHLCKE";
    if (!dni || strlen(dni) != 9) return 0;

    int numero = 0;
    for (int i = 0; i < 8; i++) {
        if (!isdigit((unsigned char)dni[i])) return 0;
        numero = numero * 10 + (dni[i] - '0');
    }

    char letra = (char)toupper((unsigned char)dni[8]);
    return letra == letras[numero % 23];
}

static int fecha_es_valida(const char *fecha) {
    if (!fecha || strlen(fecha) != 10) return 0;
    if (fecha[4] != '-' || fecha[7] != '-') return 0;

    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) continue;
        if (!isdigit((unsigned char)fecha[i])) return 0;
    }

    int y = 0, m = 0, d = 0;
    if (sscanf(fecha, "%4d-%2d-%2d", &y, &m, &d) != 3) return 0;
    if (y < 1900 || m < 1 || m > 12 || d < 1) return 0;

    int dias_mes[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    int bisiesto = ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
    if (bisiesto) dias_mes[1] = 29;
    return d <= dias_mes[m - 1];
}

static void fecha_hoy_iso(char *dest, size_t n) {
    time_t ahora = time(NULL);
    struct tm *t = localtime(&ahora);
    strftime(dest, n, "%Y-%m-%d", t);
}

static int fecha_es_igual_o_posterior(const char *base, const char *candidata) {
    return strcmp(candidata, base) >= 0;
}

static int licencia_activa_duplicada(const char *dni, int id_tipo) {
    const char *sql =
        "SELECT COUNT(*) FROM Licencia "
        "WHERE dni_ciudadano=? AND id_tipo=? "
        "AND estado IN ('En revision','Aprobada');";
    sqlite3_stmt *stmt = NULL;
    int existe = 0;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    sqlite3_bind_text(stmt, 1, dni, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, id_tipo);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        existe = sqlite3_column_int(stmt, 0) > 0;
    }
    sqlite3_finalize(stmt);
    return existe;
}

static int transicion_estado_valida(const char *actual, const char *nuevo) {
    if (!actual || !nuevo) return 0;
    if (strcmp(actual, nuevo) == 0) return 0;

    if (strcmp(actual, "En revision") == 0) {
        return strcmp(nuevo, "Aprobada") == 0 || strcmp(nuevo, "Denegada") == 0;
    }
    if (strcmp(actual, "Aprobada") == 0) {
        return strcmp(nuevo, "Caducada") == 0;
    }
    return 0;
}

static void licencias_marcar_caducadas() {
    db_ejecutar(
        "UPDATE Licencia "
        "SET estado='Caducada' "
        "WHERE estado='Aprobada' "
        "AND date(fecha_expiracion) < date('now');"
    );
}

static int cadena_vacia(const char *s) {
    if (!s) return 1;
    while (*s) {
        if (!isspace((unsigned char)*s)) return 0;
        s++;
    }
    return 1;
}

// ─── LISTAR LICENCIAS ─────────────────────────────────────────────────────────

void licencias_listar() {
    licencias_marcar_caducadas();

    int opcion;
    printf("\n--- CONSULTAR LICENCIAS ---\n");
    printf("1. Ver todas\n");
    printf("2. Filtrar por estado\n");
    printf("Seleccion: ");

    if (scanf("%d", &opcion) != 1) {
        limpiarBuffer();
        return;
    }
    limpiarBuffer();

    char sql[512];
    int total = 0;
    char *err = NULL;

    if (opcion == 1) {
        snprintf(sql, sizeof(sql),
            "SELECT l.id_licencia, t.nombre, l.dni_ciudadano, l.estado, "
            "l.fecha_solicitud, l.fecha_expiracion "
            "FROM Licencia l "
            "JOIN TipoLicencia t ON l.id_tipo = t.id_tipo;");

    } else if (opcion == 2) {
        printf("\n1. En revision\n2. Aprobada\n3. Denegada\n4. Caducada\n");
        printf("Seleccion: ");

        int filtro;
        if (scanf("%d", &filtro) != 1) {
            limpiarBuffer();
            return;
        }
        limpiarBuffer();

        const char *estado = NULL;
        switch (filtro) {
            case 1: estado = "En revision"; break;
            case 2: estado = "Aprobada";    break;
            case 3: estado = "Denegada";    break;
            case 4: estado = "Caducada";    break;
            default:
                printf("[!] Opcion invalida.\n");
                return;
        }

        snprintf(sql, sizeof(sql),
            "SELECT l.id_licencia, t.nombre, l.dni_ciudadano, l.estado, "
            "l.fecha_solicitud, l.fecha_expiracion "
            "FROM Licencia l "
            "JOIN TipoLicencia t ON l.id_tipo = t.id_tipo "
            "WHERE l.estado='%s';", estado);
    } else {
        printf("[!] Opcion invalida.\n");
        return;
    }

    printf("\n");
    int resultado = sqlite3_exec(db, sql, callback_listar_licencias, &total, &err);
    if (resultado != SQLITE_OK) {
        printf("[ERROR] %s\n", err);
        sqlite3_free(err);
        return;
    }
    if (total == 0)
        printf("[INFO] No hay licencias que mostrar.\n");
}

// ─── REGISTRAR LICENCIA ───────────────────────────────────────────────────────

void licencia_registrar() {
    licencias_marcar_caducadas();

    printf("\n--- REGISTRAR NUEVA SOLICITUD ---\n");

    int total = 0;
    char *err = NULL;
    printf("Tipos de licencia disponibles:\n");
    int resultado = sqlite3_exec(db,
        "SELECT id_tipo, nombre, descripcion, requisitos, activo "
        "FROM TipoLicencia WHERE activo=1;",
        callback_listar_tipos, &total, &err);

    if (resultado != SQLITE_OK) {
        printf("[ERROR] %s\n", err);
        sqlite3_free(err);
        return;
    }
    if (total == 0) {
        printf("[INFO] No hay tipos de licencia activos. Crea uno primero.\n");
        return;
    }

    int id_tipo;
    char dni[32];
    char fecha_expiracion[32];

    printf("\nID del tipo de licencia: ");
    if (scanf("%d", &id_tipo) != 1) {
        printf("[ERROR] ID no valido.\n");
        limpiarBuffer();
        return;
    }
    limpiarBuffer();

    // Comprobar que el tipo existe y esta activo
    char sql_check[128];
    snprintf(sql_check, sizeof(sql_check),
        "SELECT COUNT(*) FROM TipoLicencia WHERE id_tipo=%d AND activo=1;", id_tipo);
    int existe = 0;
    sqlite3_exec(db, sql_check, callback_existe, &existe, NULL);
    if (!existe) {
        printf("[ERROR] No existe ningun tipo de licencia activo con ese ID.\n");
        return;
    }

    printf("DNI del ciudadano: ");
    scanf(" %31[^\n]", dni);
    limpiarBuffer();

    printf("Fecha de expiracion (YYYY-MM-DD): ");
    scanf(" %31[^\n]", fecha_expiracion);
    limpiarBuffer();

    char fecha_solicitud[32];
    fecha_hoy_iso(fecha_solicitud, sizeof(fecha_solicitud));

    if (!dni_es_valido(dni)) {
        printf("[ERROR] DNI invalido. Formato esperado: 12345678Z.\n");
        return;
    }

    if (!fecha_es_valida(fecha_expiracion)) {
        printf("[ERROR] Fecha de expiracion invalida. Usa YYYY-MM-DD.\n");
        return;
    }

    if (!fecha_es_igual_o_posterior(fecha_solicitud, fecha_expiracion)) {
        printf("[ERROR] La fecha de expiracion no puede ser anterior a hoy (%s).\n", fecha_solicitud);
        return;
    }

    if (licencia_activa_duplicada(dni, id_tipo)) {
        printf("[ERROR] Ya existe una licencia activa o en revision para ese DNI y tipo.\n");
        return;
    }

    char sql[512];
    snprintf(sql, sizeof(sql),
        "INSERT INTO Licencia (id_tipo, dni_ciudadano, estado, fecha_solicitud, fecha_expiracion) "
        "VALUES (%d, '%s', 'En revision', '%s', '%s');",
        id_tipo, dni, fecha_solicitud, fecha_expiracion);

    if (db_ejecutar(sql)) {
        printf("[OK] Solicitud registrada correctamente para el DNI %s.\n", dni);
        char msg[300];
        snprintf(msg, sizeof(msg),
            "Ha registrado una nueva solicitud de licencia para el DNI %s", dni);
        log_escribir(msg);
    } else {
        printf("[ERROR] No se pudo registrar la solicitud.\n");
    }
}

// ─── GESTIONAR LICENCIA (cambiar estado / eliminar) ───────────────────────────

void licencia_gestionar() {
    licencias_marcar_caducadas();

    printf("\n--- GESTIONAR LICENCIA ---\n");

    int total = 0;
    char *err = NULL;
    int resultado = sqlite3_exec(db,
        "SELECT l.id_licencia, t.nombre, l.dni_ciudadano, l.estado, "
        "l.fecha_solicitud, l.fecha_expiracion "
        "FROM Licencia l "
        "JOIN TipoLicencia t ON l.id_tipo = t.id_tipo;",
        callback_listar_licencias, &total, &err);

    if (resultado != SQLITE_OK) {
        printf("[ERROR] %s\n", err);
        sqlite3_free(err);
        return;
    }
    if (total == 0) {
        printf("[INFO] No hay licencias registradas.\n");
        return;
    }

    int id;
    printf("\nID de la licencia: ");
    if (scanf("%d", &id) != 1) {
        printf("[ERROR] ID no valido.\n");
        limpiarBuffer();
        return;
    }
    limpiarBuffer();

    // Comprobar que existe
    char sql_check[128];
    snprintf(sql_check, sizeof(sql_check),
        "SELECT COUNT(*) FROM Licencia WHERE id_licencia=%d;", id);
    int existe = 0;
    sqlite3_exec(db, sql_check, callback_existe, &existe, NULL);
    if (!existe) {
        printf("[ERROR] No existe ninguna licencia con ID %d.\n", id);
        return;
    }

    int accion;
    printf("\n1. Cambiar estado\n");
    printf("2. Eliminar\n");
    printf("0. Cancelar\n");
    printf("Seleccion: ");

    if (scanf("%d", &accion) != 1) {
        limpiarBuffer();
        return;
    }
    limpiarBuffer();

    // --- CAMBIAR ESTADO ---
    if (accion == 1) {
        char estado_actual[32] = {0};
        sqlite3_stmt *stmt_estado = NULL;
        const char *sql_estado = "SELECT estado FROM Licencia WHERE id_licencia=?;";
        if (sqlite3_prepare_v2(db, sql_estado, -1, &stmt_estado, NULL) != SQLITE_OK) {
            printf("[ERROR] No se pudo consultar el estado actual.\n");
            return;
        }
        sqlite3_bind_int(stmt_estado, 1, id);
        if (sqlite3_step(stmt_estado) == SQLITE_ROW) {
            const unsigned char *txt = sqlite3_column_text(stmt_estado, 0);
            if (txt) snprintf(estado_actual, sizeof(estado_actual), "%s", txt);
        }
        sqlite3_finalize(stmt_estado);

        if (estado_actual[0] == '\0') {
            printf("[ERROR] No se pudo determinar el estado actual de la licencia.\n");
            return;
        }

        if (strcmp(estado_actual, "En revision") == 0) {
            printf("\nEstado actual: En revision\n");
            printf("1. Aprobada\n2. Denegada\n");
        } else if (strcmp(estado_actual, "Aprobada") == 0) {
            printf("\nEstado actual: Aprobada\n");
            printf("1. Caducada\n");
        } else {
            printf("[ERROR] La licencia esta en estado terminal ('%s') y no admite cambios.\n", estado_actual);
            return;
        }

        printf("Nuevo estado: ");
        int opcion_estado;
        if (scanf("%d", &opcion_estado) != 1) {
            limpiarBuffer();
            return;
        }
        limpiarBuffer();

        const char *nuevo_estado = NULL;
        if (strcmp(estado_actual, "En revision") == 0) {
            if (opcion_estado == 1) nuevo_estado = "Aprobada";
            else if (opcion_estado == 2) nuevo_estado = "Denegada";
        } else if (strcmp(estado_actual, "Aprobada") == 0) {
            if (opcion_estado == 1) nuevo_estado = "Caducada";
        }

        if (!nuevo_estado || !transicion_estado_valida(estado_actual, nuevo_estado)) {
            printf("[!] Transicion no permitida desde '%s'.\n", estado_actual);
            return;
        }

        char sql[256];
        snprintf(sql, sizeof(sql),
            "UPDATE Licencia SET estado='%s' WHERE id_licencia=%d;",
            nuevo_estado, id);

        if (db_ejecutar(sql)) {
            printf("[OK] Licencia %d actualizada a '%s'.\n", id, nuevo_estado);
            char msg[200];
            snprintf(msg, sizeof(msg),
                "Ha cambiado el estado de la licencia ID %d a '%s'", id, nuevo_estado);
            log_escribir(msg);
        } else {
            printf("[ERROR] No se pudo actualizar el estado.\n");
        }

    // --- ELIMINAR ---
    } else if (accion == 2) {
        char confirmar;
        printf("[CONFIRMAR] Esta accion elimina la licencia %d. Continuar? (s/n): ", id);
        if (scanf(" %c", &confirmar) != 1) {
            limpiarBuffer();
            return;
        }
        limpiarBuffer();
        confirmar = (char)tolower((unsigned char)confirmar);
        if (confirmar != 's') {
            printf("[INFO] Eliminacion cancelada.\n");
            return;
        }

        char sql[128];
        snprintf(sql, sizeof(sql),
            "DELETE FROM Licencia WHERE id_licencia=%d;", id);

        if (db_ejecutar(sql)) {
            if (sqlite3_changes(db) > 0) {
                printf("[OK] Licencia %d eliminada correctamente.\n", id);
                char msg[100];
                snprintf(msg, sizeof(msg), "Ha eliminado la licencia con ID %d", id);
                log_escribir(msg);
            } else {
                printf("[ERROR] No se encontro la licencia con ID %d.\n", id);
            }
        } else {
            printf("[ERROR] No se pudo eliminar la licencia.\n");
        }
    } else if (accion != 0) {
        printf("[!] Opcion invalida.\n");
    }
}

// ─── GESTIONAR TIPOS DE LICENCIA ─────────────────────────────────────────────

void tipo_licencia_gestionar() {
    int opcion;
    do {
        printf("\n--- TIPOS DE LICENCIA ---\n");
        printf("1. Listar tipos\n");
        printf("2. Crear nuevo tipo\n");
        printf("3. Dar de baja / reactivar tipo\n");
        printf("0. Volver\n");
        printf("Seleccion: ");

        if (scanf("%d", &opcion) != 1) {
            limpiarBuffer();
            opcion = -1;
            continue;
        }
        limpiarBuffer();

        if (opcion == 1) {
            printf("\n--- LISTADO DE TIPOS ---\n");
            int total = 0;
            char *err = NULL;
            sqlite3_exec(db,
                "SELECT id_tipo, nombre, descripcion, requisitos, activo FROM TipoLicencia;",
                callback_listar_tipos, &total, &err);
            if (total == 0)
                printf("[INFO] No hay tipos de licencia registrados.\n");

        } else if (opcion == 2) {
            char nombre[128], descripcion[256], requisitos[256];

            printf("Nombre del tipo: ");
            scanf(" %127[^\n]", nombre);
            limpiarBuffer();

            printf("Descripcion: ");
            scanf(" %255[^\n]", descripcion);
            limpiarBuffer();

            printf("Requisitos: ");
            scanf(" %255[^\n]", requisitos);
            limpiarBuffer();

            if (cadena_vacia(nombre)) {
                printf("[ERROR] El nombre del tipo no puede estar vacio.\n");
                continue;
            }

            sqlite3_stmt *stmt_dup = NULL;
            const char *sql_dup = "SELECT COUNT(*) FROM TipoLicencia WHERE nombre=?;";
            int existe_nombre = 0;
            if (sqlite3_prepare_v2(db, sql_dup, -1, &stmt_dup, NULL) == SQLITE_OK) {
                sqlite3_bind_text(stmt_dup, 1, nombre, -1, SQLITE_TRANSIENT);
                if (sqlite3_step(stmt_dup) == SQLITE_ROW) {
                    existe_nombre = sqlite3_column_int(stmt_dup, 0) > 0;
                }
                sqlite3_finalize(stmt_dup);
            }
            if (existe_nombre) {
                printf("[ERROR] Ya existe un tipo con ese nombre.\n");
                continue;
            }

            char sql[768];
            snprintf(sql, sizeof(sql),
                "INSERT INTO TipoLicencia (nombre, descripcion, requisitos, activo) "
                "VALUES ('%s', '%s', '%s', 1);",
                nombre, descripcion, requisitos);

            if (db_ejecutar(sql)) {
                printf("[OK] Tipo '%s' creado correctamente.\n", nombre);
                char msg[200];
                snprintf(msg, sizeof(msg), "Ha creado el tipo de licencia '%s'", nombre);
                log_escribir(msg);
            } else {
                printf("[ERROR] No se pudo crear el tipo.\n");
            }

        } else if (opcion == 3) {
            printf("\n--- TIPOS ACTUALES ---\n");
            int total = 0;
            char *err = NULL;
            sqlite3_exec(db,
                "SELECT id_tipo, nombre, descripcion, requisitos, activo FROM TipoLicencia;",
                callback_listar_tipos, &total, &err);
            if (total == 0) {
                printf("[INFO] No hay tipos registrados.\n");
                continue;
            }

            int id;
            printf("\nID del tipo: ");
            if (scanf("%d", &id) != 1) {
                limpiarBuffer();
                continue;
            }
            limpiarBuffer();

            // Leer estado actual con sqlite3_prepare_v2
            char sql_check[128];
            snprintf(sql_check, sizeof(sql_check),
                "SELECT activo FROM TipoLicencia WHERE id_tipo=%d;", id);

            int estado_actual = -1;
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, sql_check, -1, &stmt, NULL) == SQLITE_OK) {
                if (sqlite3_step(stmt) == SQLITE_ROW)
                    estado_actual = sqlite3_column_int(stmt, 0);
                sqlite3_finalize(stmt);
            }

            if (estado_actual == -1) {
                printf("[ERROR] No existe ningun tipo con ID %d.\n", id);
                continue;
            }

            int nuevo_estado = estado_actual ? 0 : 1;
            char sql[128];
            snprintf(sql, sizeof(sql),
                "UPDATE TipoLicencia SET activo=%d WHERE id_tipo=%d;",
                nuevo_estado, id);

            if (db_ejecutar(sql)) {
                printf("[OK] Tipo %d ahora esta %s.\n", id, nuevo_estado ? "ACTIVO" : "BAJA");
                char msg[150];
                snprintf(msg, sizeof(msg),
                    "Ha cambiado el tipo de licencia ID %d a '%s'",
                    id, nuevo_estado ? "ACTIVO" : "BAJA");
                log_escribir(msg);
            } else {
                printf("[ERROR] No se pudo cambiar el estado del tipo.\n");
            }
        }

    } while (opcion != 0);
}
