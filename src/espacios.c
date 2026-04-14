#include "../include/espacios.h"
#include "../include/db.h"
#include "../include/log.h"
#include "../include/funciones.h"
#include "../include/reservas.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void submenuEspacios()
{
    int opcion;
    do
    {
        printf("\n--- GESTION DE ESPACIOS ---\n");
        printf("1. Listar espacios\n");
        printf("2. Anadir Espacios\n");
        printf("3. Eliminar Espacio\n");
        printf("4. Ver reservas de un espacio\n");
        printf("5. Crear reserva a un ciudadano\n");
        printf("6. Cancelar Reserva\n");
        printf("7. Editar datos de las reservas\n");
        printf("8. Cambiar estado espacio (ACTIVO/BAJA)\n");
        printf("0. Volver al menu principal\n");
        printf("Seleccion: ");

        if (scanf("%d", &opcion) != 1)
        {
            limpiarBuffer();
            opcion = 0; // Forzamos un valor invalido
        }

        switch (opcion)
        {
        case 1:
            espacios_listar();
            break;
        case 2:
            espacios_anadir();
            break;
        case 3:
            eliminar_espacio();
            break;
        case 4:
            reservas_listar_por_espacio();
            break;
        case 5:
            reservas_crear_Aciudadano();
            break;
        case 6:
            reservas_cancelar();
            break;
        case 7:
            reservas_editar();
            break;
            ;
        case 8:
            espacios_cambiar_estado();
            break;
        case 0:
            printf("\nVolviendo al menu principal...\n");
            break;
        default:
            printf("\n[!] Opcion invalida. Intenta de nuevo.\n");
        }
    } while (opcion != 0);
}

static int callback_existe(void *data, int cols, char **valores, char **nombres)
{
    int *existe = (int *)data;
    if (valores[0] && atoi(valores[0]) > 0)
        *existe = 1;
    return 0;
}

static int callback_listar(void *data, int cols, char **valores, char **nombres)
{
    int ancho_id = 3 - (int)strlen(valores[0]);
    if (ancho_id < 0)
        ancho_id = 0;

    printf("  [%s]%*s%-20s | Capacidad: %4s | Precio/h: %6s€ | %-6s\n",
           valores[0],
           ancho_id + 1,
           "",
           valores[1],
           valores[2],
           valores[3],
           atoi(valores[4]) ? "ACTIVO" : "BAJA");
    return 0;
}
static int callback_estado(void *data, int cols, char **valores, char **nombres)
{
    int *estado = (int *)data;
    if (valores[0])
        *estado = atoi(valores[0]);
    return 0;
}

void espacios_listar()
{
    printf("\n--- LISTADO DE ESPACIOS ---\n");

    char *err = NULL;
    int resultado = sqlite3_exec(db,
                                 "SELECT id_espacio, nombre, capacidad, precio_hora, activo FROM Espacio;",
                                 callback_listar, NULL, &err);

    if (resultado != SQLITE_OK)
    {
        printf("[ERROR] %s\n", err);
        sqlite3_free(err);
    }
}

void espacios_anadir()
{
    char nombre[128];
    int capacidad;
    float precio;

    printf("\n--- ANADIR NUEVO ESPACIO ---\n");

    do
    {
        printf("Nombre: ");
        scanf(" %127[^\n]", nombre);
        limpiarBuffer();

        if (strlen(nombre) == 0)
        {
            printf("[ERROR] Por favor, introduzca un nombre.\n");
        }

    } while (strlen(nombre) == 0);

    printf("Capacidad (personas): ");
    capacidad = obtener_entero_validado(1, 10000);

    printf("Precio por hora (€): ");
    precio = obtener_float_validado(0.0f, 99999.99f);

    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO Espacio (nombre, capacidad, precio_hora, activo) VALUES (?, ?, ?, 1);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, nombre, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, capacidad);
        sqlite3_bind_double(stmt, 3, (double)precio);

        if (sqlite3_step(stmt) == SQLITE_DONE) {
            printf("[OK] Espacio '%s' añadido correctamente.\n", nombre);

            char msg[200];
            snprintf(msg, sizeof(msg), "Ha agregado a la BD el espacio '%s'", nombre);
            log_escribir(msg);
        } else {
            printf("[ERROR] No se pudo ejecutar el guardado.\n");
        }
        sqlite3_finalize(stmt);
    } else {
        printf("[ERROR] Error al preparar la consulta.\n");
    }
}
void espacios_cambiar_estado()
{
    int id;

    printf("\n--- CAMBIAR ESTADO DE ESPACIO ---\n");

    // Listar para que el admin vea los IDs y estados
    espacios_listar();

    printf("\nID del espacio: ");
    scanf("%d", &id);

    // estado actual
    char sql_check[128];
    snprintf(sql_check, sizeof(sql_check),
             "SELECT activo FROM Espacio WHERE id_espacio=%d;", id);

    int estado_actual = -1;
    sqlite3_exec(db, sql_check, callback_estado, &estado_actual, NULL);

    if (estado_actual == -1)
    {
        printf("[ERROR] No existe ningún espacio con ese ID.\n");
        return;
    }

    // contrario
    int nuevo_estado = estado_actual ? 0 : 1;
    char sql[128];
    snprintf(sql, sizeof(sql),
             "UPDATE Espacio SET activo=%d WHERE id_espacio=%d;",
             nuevo_estado, id);

    if (db_ejecutar(sql))
    {
        printf("[OK] Espacio %d ahora está %s.\n", id, nuevo_estado ? "ACTIVO" : "BAJA");
        char msg[100]; // Sobran ajustar luego si eso
        snprintf(msg, sizeof(msg), "Ha cambiado en la BD el estado del espacio con id %d  a '%s'", id, nuevo_estado ? "ACTIVO" : "BAJA");
        log_escribir(msg);
    }
    else
        printf("[ERROR] No se pudo cambiar el estado.\n");
}

void eliminar_espacio()
{
    int id;
    printf("\n--- ELIMINAR ESPACIO ---\n");
    espacios_listar();

    printf("\nID del espacio a eliminar: ");
    if (scanf("%d", &id) != 1)
    {
        printf("[ERROR] ID no válido.\n");
        while (getchar() != '\n')
            ; // Limpiar buffer
        return;
    }

    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM Espacio WHERE id_espacio = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);

        if (sqlite3_step(stmt) == SQLITE_DONE) {
            if (sqlite3_changes(db) > 0) {
                printf("[OK] Espacio %d eliminado.\n", id);
            } else {
                printf("[ADVERTENCIA] No existe ID %d.\n", id);
            }
        }
        sqlite3_finalize(stmt);
    }
}