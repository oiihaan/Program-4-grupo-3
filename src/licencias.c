#include "../include/licencias.h"
#include "../include/db.h"
#include "../include/log.h"
#include "../include/funciones.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

void submenuLicencias()
{
    int opcion;
    do
    {
        printf("\n--- GESTION DE LICENCIAS ---\n");
        printf("1. Registrar expediente\n");
        printf("2. Gestionar expediente (estado/eliminar)\n");
        printf("3. Consultar Expedientes\n");
        printf("0. Volver al menu principal\n");
        printf("Seleccion: ");

        if (scanf("%d", &opcion) != 1)
        {
            limpiarBuffer();
            opcion = 0;
        }

        switch (opcion)
        {
        case 1:
            licencia_registrar();
            break;
        case 2:
            licencia_gestionar();
            break;
        case 3:
            submenuConsultaLicencias();
            break;
        case 0:
            printf("\nVolviendo al menu principal...\n");
            break;
        default:
            printf("\n[!] Opcion invalida. Intenta de nuevo.\n");
        }
    } while (opcion != 0);
}

void submenuConsultaLicencias()
{
    int opcion;
    do
    {
        printf("\n--- CONSULTA DE EXPEDIENTES ---\n");
        printf("1. Ver licencias\n");
        printf("2. Tipos de licencia\n");
        printf("0. Volver\n");
        printf("Seleccion: ");

        if (scanf("%d", &opcion) != 1)
        {
            limpiarBuffer();
            opcion = 0;
        }

        switch (opcion)
        {
        case 1:
            licencias_listar();
            break;
        case 2:
            tipo_licencia_gestionar();
            break;
        case 0:
            printf("\nVolviendo...\n");
            break;
        default:
            printf("\n[!] Opcion invalida.\n");
        }
    } while (opcion != 0);
}

// ─── CALLBACKS ────────────────────────────────────────────────────────────────
static int callback_listar_licencias(void *data, int cols, char **valores, char **nombres)
{
    int *contador = (int *)data;
    const char *id = valores[0] ? valores[0] : "-";
    const char *tipo = valores[1] ? valores[1] : "-";
    const char *dni = valores[2] ? valores[2] : "-";
    const char *estado = valores[3] ? valores[3] : "-";
    const char *f_solic = valores[4] ? valores[4] : "-";
    const char *f_expir = valores[5] ? valores[5] : "-";

    (*contador)++;
    printf("  [%s] %-20s | DNI: %-12s | %-12s | Solicitud: %-10s | Expira: %-10s\n",
           id, tipo, dni, estado, f_solic, f_expir);
    return 0;
}

static int callback_listar_tipos(void *data, int cols, char **valores, char **nombres)
{
    int *contador = (int *)data;
    const char *id = valores[0] ? valores[0] : "-";
    const char *nombre = valores[1] ? valores[1] : "-";
    const char *desc = valores[2] ? valores[2] : "-";
    const char *req = valores[3] ? valores[3] : "-";
    const char *activo = valores[4] ? valores[4] : "0";

    (*contador)++;
    printf("  [%s] %-20s | %-30s | Requisitos: %-20s | %s\n",
           id, nombre, desc, req, atoi(activo) ? "ACTIVO" : "BAJA");
    return 0;
}

static int callback_existe(void *data, int cols, char **valores, char **nombres)
{
    int *existe = (int *)data;
    if (valores[0] && atoi(valores[0]) > 0)
        *existe = 1;
    return 0;
}

static void fecha_hoy_iso(char *dest, size_t n)
{
    time_t ahora = time(NULL);
    struct tm *t = localtime(&ahora);
    strftime(dest, n, "%Y-%m-%d", t);
}

static int licencia_activa_duplicada(const char *dni, int id_tipo)
{
    const char *sql =
        "SELECT COUNT(*) FROM Licencia "
        "WHERE dni_ciudadano=? AND id_tipo=? "
        "AND estado IN ('En revision','Aprobada');";
    sqlite3_stmt *stmt = NULL;
    int existe = 0;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        return 0;
    }
    sqlite3_bind_text(stmt, 1, dni, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, id_tipo);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        existe = sqlite3_column_int(stmt, 0) > 0;
    }
    sqlite3_finalize(stmt);
    return existe;
}

static int transicion_estado_valida(const char *actual, const char *nuevo)
{
    if (!actual || !nuevo)
        return 0;
    if (strcmp(actual, nuevo) == 0)
        return 0;

    if (strcmp(actual, "En revision") == 0)
    {
        return strcmp(nuevo, "Aprobada") == 0 || strcmp(nuevo, "Denegada") == 0;
    }
    if (strcmp(actual, "Aprobada") == 0)
    {
        return strcmp(nuevo, "Caducada") == 0;
    }
    return 0;
}

static void licencias_marcar_caducadas()
{
    db_ejecutar(
        "UPDATE Licencia "
        "SET estado='Caducada' "
        "WHERE estado='Aprobada' "
        "AND date(fecha_expiracion) < date('now');");
}

static int cadena_vacia(const char *s)
{
    if (!s)
        return 1;
    while (*s)
    {
        if (!isspace((unsigned char)*s))
            return 0;
        s++;
    }
    return 1;
}

// ─── LISTAR LICENCIAS ─────────────────────────────────────────────────────────

void licencias_listar()
{
    licencias_marcar_caducadas();

    int opcion;
    printf("\n--- CONSULTAR LICENCIAS ---\n");
    printf("1. Ver todas\n");
    printf("2. Filtrar por estado\n");
    printf("Seleccion: ");

    if (scanf("%d", &opcion) != 1)
    {
        limpiarBuffer();
        return;
    }
    limpiarBuffer();

    sqlite3_stmt *stmt;
    char *err = NULL;
    int total = 0;

    if (opcion == 1)
    {
        const char *sql = "SELECT l.id_licencia, t.nombre, l.dni_ciudadano, l.estado, "
                          "l.fecha_solicitud, l.fecha_expiracion "
                          "FROM Licencia l "
                          "JOIN TipoLicencia t ON l.id_tipo = t.id_tipo;";
        
        sqlite3_exec(db, sql, callback_listar_licencias, &total, &err);
    }
    else if (opcion == 2)
    {
        printf("\n1. En revision\n2. Aprobada\n3. Denegada\n4. Caducada\n");
        printf("Seleccion: ");

        int filtro;
        if (scanf("%d", &filtro) != 1)
        {
            limpiarBuffer();
            return;
        }
        limpiarBuffer();

        const char *estado = NULL;
        switch (filtro)
        {
        case 1: estado = "En revision"; break;
        case 2: estado = "Aprobada"; break;
        case 3: estado = "Denegada"; break;
        case 4: estado = "Caducada"; break;
        default: printf("[!] Opcion invalida.\n"); return;
        }

        const char *sql = "SELECT l.id_licencia, t.nombre, l.dni_ciudadano, l.estado, "
                          "l.fecha_solicitud, l.fecha_expiracion "
                          "FROM Licencia l "
                          "JOIN TipoLicencia t ON l.id_tipo = t.id_tipo "
                          "WHERE l.estado=?;";

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK)
        {
            sqlite3_bind_text(stmt, 1, estado, -1, SQLITE_STATIC);
            
            // Usamos un bucle para procesar los resultados simulando el callback
            while (sqlite3_step(stmt) == SQLITE_ROW)
            {
                char *valores[6];
                for (int i = 0; i < 6; i++) {
                    valores[i] = (char *)sqlite3_column_text(stmt, i);
                }
                callback_listar_licencias(&total, 6, valores, NULL);
            }
            sqlite3_finalize(stmt);
        }
    }

    if (total == 0)
        printf("[INFO] No hay licencias que mostrar.\n");
}

// ─── REGISTRAR LICENCIA ───────────────────────────────────────────────────────

void licencia_registrar()
{
    char fecha_solicitud[32];
    fecha_hoy_iso(fecha_solicitud, sizeof(fecha_solicitud));
    licencias_marcar_caducadas();

    printf("\n--- REGISTRAR NUEVA SOLICITUD ---\n");

    int total = 0;
    char *err = NULL;
    printf("Tipos de licencia disponibles:\n");
    sqlite3_exec(db, "SELECT id_tipo, nombre, descripcion, requisitos, activo "
                     "FROM TipoLicencia WHERE activo=1;", callback_listar_tipos, &total, &err);

    if (total == 0)
    {
        printf("[INFO] No hay tipos de licencia activos. Crea uno primero.\n");
        return;
    }

    int id_tipo;
    char dni[32];
    char fecha_expiracion[32];

    printf("\nID del tipo de licencia: ");
    if (scanf("%d", &id_tipo) != 1) { limpiarBuffer(); return; }
    limpiarBuffer();

    // Comprobar que el tipo existe y está activo
    sqlite3_stmt *stmt_check;
    int existe = 0;
    const char *sql_check = "SELECT COUNT(*) FROM TipoLicencia WHERE id_tipo=? AND activo=1;";
    if (sqlite3_prepare_v2(db, sql_check, -1, &stmt_check, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt_check, 1, id_tipo);
        if (sqlite3_step(stmt_check) == SQLITE_ROW) existe = sqlite3_column_int(stmt_check, 0);
        sqlite3_finalize(stmt_check);
    }

    if (!existe)
    {
        printf("[ERROR] No existe ningun tipo de licencia activo con ese ID.\n");
        return;
    }

    do {
        printf("DNI del ciudadano: ");
        scanf(" %31[^\n]", dni);
        limpiarBuffer();
    } while (!dni_es_valido(dni));

    do {
        printf("Fecha de expiracion (YYYY-MM-DD): ");
        scanf(" %31[^\n]", fecha_expiracion);
        limpiarBuffer();
    } while (!fecha_es_valida(fecha_expiracion) || !fecha_es_hoy_o_posterior(fecha_expiracion));

    if (licencia_activa_duplicada(dni, id_tipo))
    {
        printf("[ERROR] Ya existe una licencia activa o en revision para ese DNI y tipo.\n");
        return;
    }

    // CORRECCIÓN SQL INJECTION: Inserción segura
    sqlite3_stmt *stmt_ins;
    const char *sql_ins = "INSERT INTO Licencia (id_tipo, dni_ciudadano, estado, fecha_solicitud, fecha_expiracion) "
                          "VALUES (?, ?, 'En revision', ?, ?);";

    if (sqlite3_prepare_v2(db, sql_ins, -1, &stmt_ins, NULL) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt_ins, 1, id_tipo);
        sqlite3_bind_text(stmt_ins, 2, dni, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt_ins, 3, fecha_solicitud, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt_ins, 4, fecha_expiracion, -1, SQLITE_STATIC);

        if (sqlite3_step(stmt_ins) == SQLITE_DONE)
        {
            printf("[OK] Solicitud registrada correctamente.\n");
            char msg[300];
            snprintf(msg, sizeof(msg), "Ha registrado una solicitud de licencia para DNI %s", dni);
            log_escribir(msg);
        }
        sqlite3_finalize(stmt_ins);
    }
}

// ─── GESTIONAR LICENCIA (cambiar estado / eliminar) ───────────────────────────

void licencia_gestionar()
{
    licencias_marcar_caducadas();
    printf("\n--- GESTIONAR LICENCIA ---\n");

    int total = 0;
    sqlite3_exec(db, "SELECT l.id_licencia, t.nombre, l.dni_ciudadano, l.estado, "
                     "l.fecha_solicitud, l.fecha_expiracion FROM Licencia l "
                     "JOIN TipoLicencia t ON l.id_tipo = t.id_tipo;", callback_listar_licencias, &total, NULL);

    if (total == 0) return;

    int id;
    printf("\nID de la licencia: ");
    if (scanf("%d", &id) != 1) { limpiarBuffer(); return; }
    limpiarBuffer();

    int accion;
    printf("\n1. Cambiar estado\n2. Eliminar\n0. Cancelar\nSeleccion: ");
    if (scanf("%d", &accion) != 1) { limpiarBuffer(); return; }
    limpiarBuffer();

    if (accion == 1)
    {
        char estado_actual[32] = {0};
        sqlite3_stmt *stmt_est;
        const char *sql_est = "SELECT estado FROM Licencia WHERE id_licencia=?;";
        if (sqlite3_prepare_v2(db, sql_est, -1, &stmt_est, NULL) == SQLITE_OK) {
            sqlite3_bind_int(stmt_est, 1, id);
            if (sqlite3_step(stmt_est) == SQLITE_ROW) {
                const unsigned char *txt = sqlite3_column_text(stmt_est, 0);
                if (txt) strcpy(estado_actual, (char*)txt);
            }
            sqlite3_finalize(stmt_est);
        }

        if (estado_actual[0] == '\0') { printf("[ERROR] ID no encontrado.\n"); return; }

        const char *nuevo_estado = NULL;
        int opcion_estado;

        if (strcmp(estado_actual, "En revision") == 0) {
            printf("\n1. Aprobada\n2. Denegada\nSeleccion: ");
            scanf("%d", &opcion_estado); limpiarBuffer();
            nuevo_estado = (opcion_estado == 1) ? "Aprobada" : (opcion_estado == 2) ? "Denegada" : NULL;
        } else if (strcmp(estado_actual, "Aprobada") == 0) {
            printf("\n1. Caducada\nSeleccion: ");
            scanf("%d", &opcion_estado); limpiarBuffer();
            if (opcion_estado == 1) nuevo_estado = "Caducada";
        }

        if (nuevo_estado) {
            sqlite3_stmt *stmt_upd;
            const char *sql_upd = "UPDATE Licencia SET estado=? WHERE id_licencia=?;";
            if (sqlite3_prepare_v2(db, sql_upd, -1, &stmt_upd, NULL) == SQLITE_OK) {
                sqlite3_bind_text(stmt_upd, 1, nuevo_estado, -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt_upd, 2, id);
                if (sqlite3_step(stmt_upd) == SQLITE_DONE) {
                    printf("[OK] Estado actualizado a %s.\n", nuevo_estado);
                    log_escribir("Estado de licencia actualizado.");
                }
                sqlite3_finalize(stmt_upd);
            }
        }
    }
    else if (accion == 2)
    {
        sqlite3_stmt *stmt_del;
        const char *sql_del = "DELETE FROM Licencia WHERE id_licencia=?;";
        if (sqlite3_prepare_v2(db, sql_del, -1, &stmt_del, NULL) == SQLITE_OK) {
            sqlite3_bind_int(stmt_del, 1, id);
            if (sqlite3_step(stmt_del) == SQLITE_DONE && sqlite3_changes(db) > 0) {
                printf("[OK] Licencia eliminada.\n");
            }
            sqlite3_finalize(stmt_del);
        }
    }
}

// ─── GESTIONAR TIPOS DE LICENCIA ─────────────────────────────────────────────

void tipo_licencia_gestionar()
{
    int opcion;
    do
    {
        printf("\n--- TIPOS DE LICENCIA ---\n");
        printf("1. Listar tipos\n2. Crear nuevo tipo\n3. Dar de baja / reactivar tipo\n0. Volver\nSeleccion: ");
        if (scanf("%d", &opcion) != 1) { limpiarBuffer(); continue; }
        limpiarBuffer();

        if (opcion == 2)
        {
            char nombre[128], descripcion[256], requisitos[256];
            printf("Nombre: "); scanf(" %127[^\n]", nombre); limpiarBuffer();
            printf("Descripcion: "); scanf(" %255[^\n]", descripcion); limpiarBuffer();
            printf("Requisitos: "); scanf(" %255[^\n]", requisitos); limpiarBuffer();

            sqlite3_stmt *stmt_ins;
            const char *sql_ins = "INSERT INTO TipoLicencia (nombre, descripcion, requisitos, activo) VALUES (?, ?, ?, 1);";
            if (sqlite3_prepare_v2(db, sql_ins, -1, &stmt_ins, NULL) == SQLITE_OK) {
                sqlite3_bind_text(stmt_ins, 1, nombre, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt_ins, 2, descripcion, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt_ins, 3, requisitos, -1, SQLITE_STATIC);
                if (sqlite3_step(stmt_ins) == SQLITE_DONE) printf("[OK] Tipo creado.\n");
                sqlite3_finalize(stmt_ins);
            }
        }
        else if (opcion == 3)
        {
            int id;
            printf("ID del tipo: "); scanf("%d", &id); limpiarBuffer();

            sqlite3_stmt *stmt_upd;
            const char *sql_upd = "UPDATE TipoLicencia SET activo = NOT activo WHERE id_tipo=?;";
            if (sqlite3_prepare_v2(db, sql_upd, -1, &stmt_upd, NULL) == SQLITE_OK) {
                sqlite3_bind_int(stmt_upd, 1, id);
                if (sqlite3_step(stmt_upd) == SQLITE_DONE) printf("[OK] Estado alternado.\n");
                sqlite3_finalize(stmt_upd);
            }
        }
    } while (opcion != 0);
}