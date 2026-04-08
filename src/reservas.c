#include "../include/reservas.h"
#include "../include/db.h"
#include "../include/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ===== CALLBACKS PARA CONSULTAS ===== */

static int callback_listar_reservas(void *data, int cols, char **valores, char **nombres) {
    printf("  [ID: %s] Espacio: %s | DNI: %s | Fecha: %s | Entrada: %s - Salida: %s\n",
        valores[0],              // id_reserva
        valores[1],              // id_espacio
        valores[2],              // dni_ciudadano
        valores[3],              // fecha
        valores[4],              // franja_inicio
        valores[5]               // franja_fin
    );
    printf("    Personas: %s | Estado: %s\n",
        valores[6],              // num_personas
        atoi(valores[7]) ? "CANCELADA" : "ACTIVA"  // cancelada
    );
    printf("  ---\n");
    return 0;
}

static int callback_contar_reservas(void *data, int cols, char **valores, char **nombres) {
    int *total = (int *)data;
    if (valores[0]) {
        *total = atoi(valores[0]);
    }
    return 0;
}

static int callback_obtener_id(void *data, int cols, char **valores, char **nombres) {
    int *id = (int *)data;
    if (valores[0]) {
        *id = atoi(valores[0]);
    }
    return 0;
}

/* ===== FUNCIONES CRUD ===== */

/* CREATE - Crear nueva reserva */
void reservas_crear() {
    int id_espacio;
    char dni_ciudadano[16];
    char fecha[16];
    char franja_inicio[6];
    char franja_fin[6];
    int num_personas;

    printf("\n--- CREAR NUEVA RESERVA ---\n");

    printf("ID del espacio: ");
    scanf("%d", &id_espacio);
    getchar();  // Limpiar buffer

    printf("DNI del ciudadano: ");
    scanf(" %15[^\n]", dni_ciudadano);
    getchar();

    printf("Fecha de reserva (YYYY-MM-DD): ");
    scanf(" %15[^\n]", fecha);
    getchar();

    printf("Hora de entrada (HH:MM): ");
    scanf(" %5[^\n]", franja_inicio);
    getchar();

    printf("Hora de salida (HH:MM): ");
    scanf(" %5[^\n]", franja_fin);
    getchar();

    printf("Número de personas: ");
    scanf("%d", &num_personas);
    getchar();

    // Verificar que el espacio existe y está activo
    char sql_check[256];
    snprintf(sql_check, sizeof(sql_check),
        "SELECT COUNT(*) FROM Espacio WHERE id_espacio=%d AND activo=1;", id_espacio);

    int existe = 0;
    sqlite3_exec(db, sql_check, callback_contar_reservas, &existe, NULL);

    if (existe == 0) {
        printf("[ERROR] El espacio con ID %d no existe o está dado de baja.\n", id_espacio);
        return;
    }

    // Insertar la reserva
    char sql[512];
    snprintf(sql, sizeof(sql),
        "INSERT INTO Reserva (id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin, num_personas, cancelada) "
        "VALUES (%d, '%s', '%s', '%s', '%s', %d, 0);",
        id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin, num_personas);

    if (db_ejecutar(sql)) {
        printf("[OK] Reserva creada correctamente.\n");
        printf("     Espacio: %d | DNI: %s | Fecha: %s | %s-%s\n",
            id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin);

        char msg[256];
        snprintf(msg, sizeof(msg),
            "Ha creado una nueva reserva para el espacio %d (DNI: %s, Fecha: %s)",
            id_espacio, dni_ciudadano, fecha);
        log_escribir(msg);
    } else {
        printf("[ERROR] No se pudo crear la reserva.\n");
    }
}

/* READ - Listar todas las reservas */
void reservas_listar_todas() {
    printf("\n--- LISTADO DE TODAS LAS RESERVAS ---\n");

    char *err = NULL;
    int resultado = sqlite3_exec(db,
        "SELECT id_reserva, id_espacio, dni_ciudadano, fecha, franja_inicio, "
        "franja_fin, num_personas, cancelada FROM Reserva ORDER BY fecha DESC;",
        callback_listar_reservas, NULL, &err);

    if (resultado != SQLITE_OK) {
        printf("[ERROR] %s\n", err);
        sqlite3_free(err);
    }

    // Mostrar estadísticas
    int total_reservas = 0;
    sqlite3_exec(db, "SELECT COUNT(*) FROM Reserva;", callback_contar_reservas, &total_reservas, NULL);

    int activas = 0;
    sqlite3_exec(db, "SELECT COUNT(*) FROM Reserva WHERE cancelada=0;", callback_contar_reservas, &activas, NULL);

    printf("  Total de reservas: %d | Activas: %d | Canceladas: %d\n",
        total_reservas, activas, total_reservas - activas);
}

/* READ - Listar reservas de un espacio específico */
void reservas_listar_por_espacio() {
    int id_espacio;

    printf("\n--- VER RESERVAS DE UN ESPACIO ---\n");
    printf("ID del espacio: ");
    scanf("%d", &id_espacio);
    getchar();

    // Verificar que el espacio existe
    char sql_check[256];
    snprintf(sql_check, sizeof(sql_check),
        "SELECT nombre FROM Espacio WHERE id_espacio=%d;", id_espacio);

    char nombre_espacio[128] = "Desconocido";

    // Callback auxiliar para obtener nombre
    struct {
        char *nombre;
    } contexto = { nombre_espacio };

    // Simplificado: ejecutar directamente
    printf("\n--- RESERVAS DEL ESPACIO %d ---\n", id_espacio);

    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT id_reserva, id_espacio, dni_ciudadano, fecha, franja_inicio, "
        "franja_fin, num_personas, cancelada FROM Reserva "
        "WHERE id_espacio=%d ORDER BY fecha DESC;",
        id_espacio);

    char *err = NULL;
    int resultado = sqlite3_exec(db, sql, callback_listar_reservas, NULL, &err);

    if (resultado != SQLITE_OK) {
        printf("[ERROR] %s\n", err);
        sqlite3_free(err);
    }

    // Estadísticas por espacio
    int total = 0;
    snprintf(sql, sizeof(sql),
        "SELECT COUNT(*) FROM Reserva WHERE id_espacio=%d;", id_espacio);
    sqlite3_exec(db, sql, callback_contar_reservas, &total, NULL);

    int activas = 0;
    snprintf(sql, sizeof(sql),
        "SELECT COUNT(*) FROM Reserva WHERE id_espacio=%d AND cancelada=0;", id_espacio);
    sqlite3_exec(db, sql, callback_contar_reservas, &activas, NULL);

    if (total == 0) {
        printf("\n[INFO] Este espacio no tiene reservas registradas.\n");
    } else {
        printf("\nTotal de reservas: %d | Activas: %d | Canceladas: %d\n",
            total, activas, total - activas);
    }
}

/* UPDATE - Cancelar una reserva */
void reservas_cancelar() {
    int id_reserva;

    printf("\n--- CANCELAR RESERVA ---\n");

    // Primero listar todas para que el usuario vea los IDs
    printf("Reservas existentes:\n");
    reservas_listar_todas();

    printf("\nID de la reserva a cancelar: ");
    scanf("%d", &id_reserva);
    getchar();

    // Verificar que la reserva existe y no está ya cancelada
    char sql_check[256];
    snprintf(sql_check, sizeof(sql_check),
        "SELECT cancelada FROM Reserva WHERE id_reserva=%d;", id_reserva);

    int estado_cancelada = -1;
    sqlite3_exec(db, sql_check, callback_obtener_id, &estado_cancelada, NULL);

    if (estado_cancelada == -1) {
        printf("[ERROR] No existe ninguna reserva con ID %d.\n", id_reserva);
        return;
    }

    if (estado_cancelada == 1) {
        printf("[ADVERTENCIA] Esta reserva ya está cancelada.\n");
        return;
    }

    // Cancelar la reserva (marcar como cancelada)
    char sql[256];
    snprintf(sql, sizeof(sql),
        "UPDATE Reserva SET cancelada=1 WHERE id_reserva=%d;", id_reserva);

    if (db_ejecutar(sql)) {
        printf("[OK] Reserva %d cancelada correctamente.\n", id_reserva);

        char msg[256];
        snprintf(msg, sizeof(msg), "Ha cancelado la reserva con ID %d", id_reserva);
        log_escribir(msg);
    } else {
        printf("[ERROR] No se pudo cancelar la reserva.\n");
    }
}

/* UPDATE - Editar detalles de una reserva */
void reservas_editar() {
    int id_reserva;
    int opcion_editar;

    printf("\n--- EDITAR RESERVA ---\n");

    // Listar todas las reservas
    printf("Reservas existentes:\n");
    reservas_listar_todas();

    printf("\nID de la reserva a editar: ");
    scanf("%d", &id_reserva);
    getchar();

    // Verificar que la reserva existe
    char sql_check[256];
    snprintf(sql_check, sizeof(sql_check),
        "SELECT id_reserva FROM Reserva WHERE id_reserva=%d;", id_reserva);

    int existe = 0;
    sqlite3_exec(db, sql_check, callback_contar_reservas, &existe, NULL);

    if (existe == 0) {
        printf("[ERROR] No existe ninguna reserva con ID %d.\n", id_reserva);
        return;
    }

    // Menú de opciones de edición
    printf("\n--- SELECCIONA QUÉ EDITAR ---\n");
    printf("1. Cambiar DNI del ciudadano\n");
    printf("2. Cambiar fecha\n");
    printf("3. Cambiar hora de entrada\n");
    printf("4. Cambiar hora de salida\n");
    printf("5. Cambiar número de personas\n");
    printf("0. Cancelar\n");
    printf("Opción: ");
    scanf("%d", &opcion_editar);
    getchar();

    char sql_update[512];
    char nuevo_valor[128];

    switch (opcion_editar) {
        case 1:
            printf("Nuevo DNI: ");
            scanf(" %127[^\n]", nuevo_valor);
            snprintf(sql_update, sizeof(sql_update),
                "UPDATE Reserva SET dni_ciudadano='%s' WHERE id_reserva=%d;",
                nuevo_valor, id_reserva);
            break;
        case 2:
            printf("Nueva fecha (YYYY-MM-DD): ");
            scanf(" %127[^\n]", nuevo_valor);
            snprintf(sql_update, sizeof(sql_update),
                "UPDATE Reserva SET fecha='%s' WHERE id_reserva=%d;",
                nuevo_valor, id_reserva);
            break;
        case 3:
            printf("Nueva hora de entrada (HH:MM): ");
            scanf(" %127[^\n]", nuevo_valor);
            snprintf(sql_update, sizeof(sql_update),
                "UPDATE Reserva SET franja_inicio='%s' WHERE id_reserva=%d;",
                nuevo_valor, id_reserva);
            break;
        case 4:
            printf("Nueva hora de salida (HH:MM): ");
            scanf(" %127[^\n]", nuevo_valor);
            snprintf(sql_update, sizeof(sql_update),
                "UPDATE Reserva SET franja_fin='%s' WHERE id_reserva=%d;",
                nuevo_valor, id_reserva);
            break;
        case 5:
            printf("Nuevo número de personas: ");
            scanf(" %127[^\n]", nuevo_valor);
            snprintf(sql_update, sizeof(sql_update),
                "UPDATE Reserva SET num_personas=%s WHERE id_reserva=%d;",
                nuevo_valor, id_reserva);
            break;
        case 0:
            printf("Edición cancelada.\n");
            return;
        default:
            printf("[ERROR] Opción no válida.\n");
            return;
    }

    if (db_ejecutar(sql_update)) {
        printf("[OK] Reserva %d actualizada correctamente.\n", id_reserva);
        char msg[256];
        snprintf(msg, sizeof(msg), "Ha editado los datos de la reserva con ID %d", id_reserva);
        log_escribir(msg);
    } else {
        printf("[ERROR] No se pudo actualizar la reserva.\n");
    }
}
