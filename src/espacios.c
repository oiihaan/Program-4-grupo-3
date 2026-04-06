#include "../include/espacios.h"
#include "../include/db.h"
#include <stdio.h>
#include <stdlib.h>

static int callback_existe(void *data, int cols, char **valores, char **nombres) {
    int *existe = (int *)data;
    if (valores[0] && atoi(valores[0]) > 0)
        *existe = 1;
    return 0;
}

static int callback_listar(void *data, int cols, char **valores, char **nombres) {
    printf("  [%s] %-20s | Capacidad: %s | Precio/h: %s€ | %s\n",
        valores[0],
        valores[1],
        valores[2],
        valores[3],
        atoi(valores[4]) ? "ACTIVO" : "BAJA"
    );
    return 0;
}
static int callback_estado(void *data, int cols, char **valores, char **nombres) {
    int *estado = (int *)data;
    if (valores[0])
        *estado = atoi(valores[0]);
    return 0;
}

void espacios_listar() {
    printf("\n--- LISTADO DE ESPACIOS ---\n");

    char *err = NULL;
    int resultado = sqlite3_exec(db,
        "SELECT id_espacio, nombre, capacidad, precio_hora, activo FROM Espacio;",
        callback_listar, NULL, &err);

    if (resultado != SQLITE_OK) {
        printf("[ERROR] %s\n", err);
        sqlite3_free(err);
    }
}

void espacios_anadir() {
    char nombre[128];
    int capacidad;
    float precio;

    printf("\n--- ANADIR NUEVO ESPACIO ---\n");

    printf("Nombre: ");
    scanf(" %127[^\n]", nombre);

    printf("Capacidad (personas): ");
    scanf("%d", &capacidad);

    printf("Precio por hora (€): ");
    scanf("%f", &precio);

    char sql[512];
    snprintf(sql, sizeof(sql),
        "INSERT INTO Espacio (nombre, capacidad, precio_hora, activo) "
        "VALUES ('%s', %d, %.2f, 1);",
        nombre, capacidad, precio);

    if (db_ejecutar(sql))
        printf("[OK] Espacio '%s' añadido correctamente.\n", nombre);
    else
        printf("[ERROR] No se pudo añadir el espacio.\n");
}
void espacios_cambiar_estado() {
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

    if (estado_actual == -1) {
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
        printf("[OK] Espacio %d ahora está %s.\n", id, nuevo_estado ? "ACTIVO" : "BAJA");
    else
        printf("[ERROR] No se pudo cambiar el estado.\n");
}