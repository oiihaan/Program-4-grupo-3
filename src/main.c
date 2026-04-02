#include <stdio.h>
#include <stdlib.h>
#include "../include/declaraciones.h"

/* --- MENU PRINCIPAL --- */

int main()
{
    int opcion;

    printf("=========================================\n");
    printf(">>> INICIO DE SESION DEL AYUNTAMIENTO <<<\n");
    printf("=========================================\n");
    printf("Autenticando trabajador...\n[OK] Acceso concedido.\n");

    do
    {
        printf("\n*** MENU PRINCIPAL DEL ADMINISTRADOR ***\n");
        printf("1. Gestion de espacios\n");
        printf("2. Gestion de noticias\n");
        printf("3. Gestion de licencias\n");
        printf("4. Configuracion\n");
        printf("5. Salir\n");
        printf("Seleccion: ");

        // evita bucle infinito si se mete texto
        if (scanf("%d", &opcion) != 1)
        {
            limpiarBuffer();
            opcion = 0;
        }

        switch (opcion)
        {
        case 1:
            submenuEspacios();
            break;
        case 2:
            submenuNoticias();
            break;
        case 3:
            submenuLicencias();
            break;
        case 4:
            submenuConfiguracion();
            break;
        case 5:
            printf("\n[INFO] Cerrando sesion local. Hasta pronto!\n");
            break;
        default:
            printf("\n[ERROR] Opcion no valida. Por favor, introduce un numero del 1 al 5.\n");
            break;
        }
    } while (opcion != 5);

    return 0;
}