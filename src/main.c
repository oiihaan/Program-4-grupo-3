#include <stdio.h>
#include <stdlib.h>
#include "../include/funciones.h"
#include "../include/espacios.h"
#include "../include/noticias.h"
#include "../include/licencias.h"
#include "../include/config.h"
#include "../include/db.h"
#include "../include/config.h"
#include "../include/auth.h"
#include "../include/log.h"


//INCLUDEs para cURL (lo de la API del tiempo)
#include <curl/curl.h>
#include "../include/noticias.h"





int main() {
    //Inicializa para que no pete la app basicamente (Prepara RAM)
    curl_global_init(CURL_GLOBAL_DEFAULT);

    //Log de encendido del sistema
    log_escribir("El sistema se ha encendido correctamente");  

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
        printf("0. Salir\n");
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
            case 0:
                 printf("\n[INFO] Cerrando sesion. Hasta pronto!\n"); 
                 log_escribir("Ha cerrado la sesion");
                 log_set_usuario("Sistema");
                 break;
            default: printf("\n[ERROR] Opcion no valida.\n"); break;
        }
    } while (opcion != 0);


    //Log de apago del sistema antes de apagar
    log_escribir("El sistema se ha apagado");

    // Limpieza global antes de salir
    curl_global_cleanup();
    printf("Degub");
    // 6. Cerrar BD
    db_cerrar();
    return 0;
}
