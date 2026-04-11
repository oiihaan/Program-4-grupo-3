#include "../include/config.h"
#include "../include/funciones.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Para que lea el fichero server.conf, y guardar sus valores en memoria para que todo el programa los pueda usar
// Es como para leer los valores de un .env
Config config;

static char apertura[6];
static char cierre[6];

void submenuConfiguracion()
{
    int opcion;
    do
    {
        printf("\n--- CONFIGURACION ---\n");
        printf("1. Gestion de contrasenas y usuarios\n");
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
            printf("\n[+] Modulo: Administracion de credenciales...\n");
            break;
        case 0:
            printf("\nVolviendo al menu principal...\n");
            break;
        default:
            printf("\n[!] Opcion invalida. Intenta de nuevo.\n");
        }
    } while (opcion != 0);
}

int config_cargar(const char *ruta)
{
    FILE *f = fopen(ruta, "r");
    if (!f)
    {
        printf("[ERROR] No se encontro el fichero de configuracion: %s\n", ruta);
        return 0;
    }

    char linea[256];
    while (fgets(linea, sizeof(linea), f))
    {
        // Para comentarios y lineas vacias:
        if (linea[0] == '#' || linea[0] == '\n')
            continue;

        char clave[128], valor[128];
        if (sscanf(linea, "%127[^=]=%127s", clave, valor) == 2)
        {
            if (strcmp(clave, "db_ruta") == 0)
                strncpy(config.db_ruta, valor, sizeof(config.db_ruta));
            else if (strcmp(clave, "admin_usuario") == 0)
                strncpy(config.admin_usuario, valor, sizeof(config.admin_usuario));
            else if (strcmp(clave, "server_puerto") == 0)
                strncpy(config.server_puerto, valor, sizeof(config.server_puerto));
            else if (strcmp(clave, "hora_apertura") == 0)
                strncpy(apertura, valor, sizeof(apertura));

            else if (strcmp(clave, "hora_cierre") == 0)
                strncpy(cierre, valor, sizeof(cierre));
        }
    }

    fclose(f);
    printf("[OK] Configuracion cargada desde: %s\n", ruta);
    return 1;
}

void config_mostrar()
{
    printf("\n--- CONFIGURACION ACTUAL ---\n");
    printf("BD:      %s\n", config.db_ruta);
    printf("Admin:   %s\n", config.admin_usuario);
    printf("Puerto:  %s\n", config.server_puerto);
}
int definir_intentos()
{
    int intentos = 3; // Valor por defecto
    FILE *f = fopen("./server.conf", "r");
    if (!f)
    {
        printf("[ERROR] No se encontro el fichero de configuracion: %s\n", "./server.conf");
        return intentos; // Valor por defecto
    }

    char linea[256];

    while (fgets(linea, sizeof(linea), f))
    {
        if (linea[0] == '#' || linea[0] == '\n')
            continue;

        char clave[128], valor[128];
        if (sscanf(linea, "%127[^=]=%127s", clave, valor) == 2)
        {
            if (strcmp(clave, "max_intentos") == 0)
            {
                intentos = atoi(valor);
                break;
            }
        }
    }

    fclose(f);
    return intentos;
}

const char *get_apertura()
{
    return apertura;
}

const char *get_cierre()
{
    return cierre;
}