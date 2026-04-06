#include <stdio.h>
#include <stdlib.h>
#include "../include/declaraciones.h"
#include "../include/db.h"
#include "../include/config.h"
#include "../include/auth.h"

int main() {
    //cargar configuración
    if (!config_cargar("./server.conf")) return 1;

    //abrir base de datos
    if (!db_abrir(config.db_ruta)) return 1;

    //tablas
    db_crear_tablas();

    // 4. Login
    if (!auth_login()) {
        db_cerrar();
        return 1;
    }

    // 5. Menú principal
    int opcion;
    do {
        printf("\n*** MENU PRINCIPAL DEL ADMINISTRADOR ***\n");
        printf("1. Gestion de espacios\n");
        printf("2. Gestion de noticias\n");
        printf("3. Gestion de licencias\n");
        printf("4. Configuracion\n");
        printf("5. Salir\n");
        printf("Seleccion: ");

        if (scanf("%d", &opcion) != 1) {
            limpiarBuffer();
            opcion = 0;
        }

        switch (opcion) {
            case 1: submenuEspacios();      break;
            case 2: submenuNoticias();      break;
            case 3: submenuLicencias();     break;
            case 4: submenuConfiguracion(); break;
            case 5: printf("\n[INFO] Cerrando sesion. Hasta pronto!\n"); break;
            default: printf("\n[ERROR] Opcion no valida.\n"); break;
        }
    } while (opcion != 5);

    // 6. Cerrar BD
    db_cerrar();
    return 0;
}