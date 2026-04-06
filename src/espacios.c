#include "../include/espacios.h"
#include "../include/db.h"
#include <stdio.h>
#include <stdlib.h>

// Callback — se llama una vez por cada espacio encontrado
static int callback_listar(void *data, int cols, char **valores, char **nombres) {
    printf("  [%s] %-20s | Capacidad: %s | Precio/h: %s€ | %s\n",
        valores[0],  // id
        valores[1],  // nombre
        valores[2],  // capacidad
        valores[3],  // precio_hora
        atoi(valores[4]) ? "ACTIVO" : "BAJA"  // activo
    );
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