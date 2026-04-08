#include "../include/funciones.h"
#include <stdio.h>
#include <stdlib.h>
#include "../include/espacios.h"
#include "../include/noticias.h"

void limpiarBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

void submenuEspacios() {
    int opcion;
    do {
        printf("\n--- GESTION DE ESPACIOS ---\n");
        printf("1. Listar espacios\n");
        printf("2. Anadir Espacios\n");
        printf("3. Eliminar Espacio\n");
        printf("4. Ver reservas de un espacio\n");
        printf("5. Cancelar Reserva\n");
        printf("6. Cambiar estado espacio (ACTIVO/BAJA)\n");
        printf("0. Volver al menu principal\n");
        printf("Seleccion: ");
        
        if (scanf("%d", &opcion) != 1) {
            limpiarBuffer();
            opcion = 0; // Forzamos un valor invalido
        }

        switch (opcion) {
            case 1: espacios_listar(); break;
            case 2: espacios_anadir(); break;
            case 3: eliminar_espacio(); break;
            case 4: printf("\n[+] Modulo: Consultando reservas de espacio...\n"); break;
            case 5: printf("\n[+] Modulo: Cancelacion de reservas...\n"); break;
            case 6: espacios_cambiar_estado(); break;
            case 0: printf("\nVolviendo al menu principal...\n"); break;
            default: printf("\n[!] Opcion invalida. Intenta de nuevo.\n");
        }
    } while (opcion != 0);
}



void submenuNoticias() {
    int opcion;
    do {
        printf("\n--- GESTION DE NOTICIAS ---\n");
        printf("1. Publicar Noticia\n");
        printf("2. Editar Noticia\n");
        printf("3. Eliminar Noticia\n");
        printf("4. Consultar Noticias\n");
        printf("0. Volver al menu principal\n");
        printf("Seleccion: ");
        
        if (scanf("%d", &opcion) != 1) {
            limpiarBuffer();
            opcion = 0;
        }

        switch (opcion) {
            case 1: noticia_publicar(); break;
            case 2: printf("\n[+] Modulo: Edicion de noticias...\n"); break;
            case 3: noticia_eliminar(); break;
            case 4: verNoticias(); break;
            case 0: printf("\nVolviendo al menu principal...\n"); break;
            default: printf("\n[!] Opcion invalida. Intenta de nuevo.\n");
        }
    } while (opcion != 0);
}

void submenuLicencias() {
    int opcion;
    do {
        printf("\n--- GESTION DE LICENCIAS ---\n");
        printf("1. Registrar expediente\n");
        printf("2. Cambiar de estado\n");
        printf("3. Consultar Expedientes\n");
        printf("0. Volver al menu principal\n");
        printf("Seleccion: ");
        
        if (scanf("%d", &opcion) != 1) {
            limpiarBuffer();
            opcion = 0;
        }

        switch (opcion) {
            case 1: printf("\n[+] Modulo: Registro de nuevo expediente...\n"); break;
            case 2: printf("\n[+] Modulo: Actualizando estado de licencia...\n"); break;
            case 3: printf("\n[+] Modulo: Busqueda de expedientes...\n"); break;
            case 0: printf("\nVolviendo al menu principal...\n"); break;
            default: printf("\n[!] Opcion invalida. Intenta de nuevo.\n");
        }
    } while (opcion != 0);
}


void submenuConfiguracion() {
    int opcion;
    do {
        printf("\n--- CONFIGURACION ---\n");
        printf("1. Gestion de contrasenas y usuarios\n");
        printf("0. Volver al menu principal\n");
        printf("Seleccion: ");
        
        if (scanf("%d", &opcion) != 1) {
            limpiarBuffer();
            opcion = 0;
        }

        switch (opcion) {
            case 1: printf("\n[+] Modulo: Administracion de credenciales...\n"); break;
            case 0: printf("\nVolviendo al menu principal...\n"); break;
            default: printf("\n[!] Opcion invalida. Intenta de nuevo.\n");
        }
    } while (opcion != 0);
}
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include "funciones.h"

char* capturar_contrasena() {
    struct termios viejo, nuevo;
    char *password = NULL;
    int i = 0;
    int ch;

    // Configuración de la terminal: desactivamos el eco (ECHO)
    tcgetattr(STDIN_FILENO, &viejo);
    nuevo = viejo;
    nuevo.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &nuevo);

    printf("Introduce tu clave: ");

    while (1) {
        ch = getchar();

        if (ch == '\n' || ch == '\r') { // Enter
            break;
        } 
        else if (ch == 127 || ch == 8) { // Backspace (Borrar)
            if (i > 0) {
                i--;
                printf("\b \b"); // Mueve el cursor atrás, imprime espacio, vuelve atrás
                password = (char *)realloc(password, (i + 1) * sizeof(char));
            }
        } 
        else {
            // Reservamos espacio para la nueva letra + el hueco del '\0'
            char *temp = (char *)realloc(password, (i + 2) * sizeof(char));
            if (temp == NULL) {
                free(password);
                return NULL;
            }
            password = temp;
            password[i++] = (char)ch;
            printf("*"); // El "engaño" visual
        }
    }

    if (password != NULL) {
        password[i] = '\0'; // Cerramos la cadena
    }

    // Restauramos la consola a su estado original
    tcsetattr(STDIN_FILENO, TCSANOW, &viejo);
    printf("\n");

    return password; // ¡Recuerda hacer free() de esto en el main!
}
