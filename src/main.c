#include <stdio.h>
#include <stdlib.h>

/*  limpia el buffer de entrada si el usuario mete letras en vez de numeros */
void limpiarBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}



void submenuEspacios() {
    int opcion;
    do {
        printf("\n--- GESTION DE ESPACIOS ---\n");
        printf("1. Anadir Espacios\n");
        printf("2. Listar espacios\n");
        printf("3. Ver reservas de un espacio\n");
        printf("4. Cancelar Reserva\n");
        printf("5. Volver al menu principal\n");
        printf("Seleccion: ");
        
        if (scanf("%d", &opcion) != 1) {
            limpiarBuffer();
            opcion = 0; // Forzamos un valor invalido
        }

        switch (opcion) {
            case 1: printf("\n[+] Modulo: Anadir nuevo espacio...\n"); break;
            case 2: printf("\n[+] Modulo: Listado de espacios disponibles...\n"); break;
            case 3: printf("\n[+] Modulo: Consultando reservas de espacio...\n"); break;
            case 4: printf("\n[+] Modulo: Cancelacion de reservas...\n"); break;
            case 5: printf("\nVolviendo al menu principal...\n"); break;
            default: printf("\n[!] Opcion invalida. Intenta de nuevo.\n");
        }
    } while (opcion != 5);
}

void submenuNoticias() {
    int opcion;
    do {
        printf("\n--- GESTION DE NOTICIAS ---\n");
        printf("1. Publicar Noticia\n");
        printf("2. Editar Noticia\n");
        printf("3. Eliminar Noticia\n");
        printf("4. Volver al menu principal\n");
        printf("Seleccion: ");
        
        if (scanf("%d", &opcion) != 1) {
            limpiarBuffer();
            opcion = 0;
        }

        switch (opcion) {
            case 1: printf("\n[+] Modulo: Redactar nueva publicacion...\n"); break;
            case 2: printf("\n[+] Modulo: Edicion de noticias...\n"); break;
            case 3: printf("\n[+] Modulo: Eliminando noticia...\n"); break;
            case 4: printf("\nVolviendo al menu principal...\n"); break;
            default: printf("\n[!] Opcion invalida. Intenta de nuevo.\n");
        }
    } while (opcion != 4);
}

void submenuLicencias() {
    int opcion;
    do {
        printf("\n--- GESTION DE LICENCIAS ---\n");
        printf("1. Registrar expediente\n");
        printf("2. Cambiar de estado\n");
        printf("3. Consultar Expedientes\n");
        printf("4. Volver al menu principal\n");
        printf("Seleccion: ");
        
        if (scanf("%d", &opcion) != 1) {
            limpiarBuffer();
            opcion = 0;
        }

        switch (opcion) {
            case 1: printf("\n[+] Modulo: Registro de nuevo expediente...\n"); break;
            case 2: printf("\n[+] Modulo: Actualizando estado de licencia...\n"); break;
            case 3: printf("\n[+] Modulo: Busqueda de expedientes...\n"); break;
            case 4: printf("\nVolviendo al menu principal...\n"); break;
            default: printf("\n[!] Opcion invalida. Intenta de nuevo.\n");
        }
    } while (opcion != 4);
}

void submenuConfiguracion() {
    int opcion;
    do {
        printf("\n--- CONFIGURACION ---\n");
        printf("1. Gestion de contrasenas y usuarios\n");
        printf("2. Volver al menu principal\n");
        printf("Seleccion: ");
        
        if (scanf("%d", &opcion) != 1) {
            limpiarBuffer();
            opcion = 0;
        }

        switch (opcion) {
            case 1: printf("\n[+] Modulo: Administracion de credenciales...\n"); break;
            case 2: printf("\nVolviendo al menu principal...\n"); break;
            default: printf("\n[!] Opcion invalida. Intenta de nuevo.\n");
        }
    } while (opcion != 2);
}

/* --- MENU PRINCIPAL --- */

int main() {
    int opcion;

    printf("=========================================\n");
    printf(">>> INICIO DE SESION DEL AYUNTAMIENTO <<<\n");
    printf("=========================================\n");
    printf("Autenticando trabajador...\n[OK] Acceso concedido.\n");

    do {
        printf("\n*** MENU PRINCIPAL DEL ADMINISTRADOR ***\n");
        printf("1. Gestion de espacios\n");
        printf("2. Gestion de noticias\n");
        printf("3. Gestion de licencias\n");
        printf("4. Configuracion\n");
        printf("5. Salir\n");
        printf("Seleccion: ");
        
        //evita bucle infinito si se mete texto
        if (scanf("%d", &opcion) != 1) {
            limpiarBuffer();
            opcion = 0; 
        }

        switch (opcion) {
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