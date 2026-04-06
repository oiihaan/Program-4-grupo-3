#include "../include/config.h"
#include <stdio.h>
#include <string.h>

//Para que lea el fichero server.conf, y guardar sus valores en memoria para que todo el programa los pueda usar
//Es como para leer los valores de un .env
Config config;

int config_cargar(const char *ruta) {
    FILE *f = fopen(ruta, "r");
    if (!f) {
        printf("[ERROR] No se encontro el fichero de configuracion: %s\n", ruta);
        return 0;
    }

    char linea[256];
    while (fgets(linea, sizeof(linea), f)) {
        // Para comentarios y lineas vacias:
        if (linea[0] == '#' || linea[0] == '\n') continue;

        char clave[128], valor[128];
        if (sscanf(linea, "%127[^=]=%127s", clave, valor) == 2) {
            if (strcmp(clave, "db_ruta") == 0)
                strncpy(config.db_ruta, valor, sizeof(config.db_ruta));
            else if (strcmp(clave, "admin_usuario") == 0)
                strncpy(config.admin_usuario, valor, sizeof(config.admin_usuario));
            else if (strcmp(clave, "server_puerto") == 0)
                strncpy(config.server_puerto, valor, sizeof(config.server_puerto));
        }
    }

    fclose(f);
    printf("[OK] Configuracion cargada desde: %s\n", ruta);
    return 1;
}

void config_mostrar() {
    printf("\n--- CONFIGURACION ACTUAL ---\n");
    printf("BD:      %s\n", config.db_ruta);
    printf("Admin:   %s\n", config.admin_usuario);
    printf("Puerto:  %s\n", config.server_puerto);
}