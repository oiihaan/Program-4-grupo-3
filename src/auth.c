#include "../include/auth.h"
#include "../include/db.h"
#include "../include/config.h"
#include "../include/funciones.h"
#include "../include/log.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//Pa los resultados del select, q sino se liaba
static int callback_login(void *data, int cols, char **valores, char **nombres) {
    int *encontrado = (int *)data;
    if (valores[0] && atoi(valores[0]) > 0)
        *encontrado = 1;
    return 0;
}

int auth_login() {
    char usuario[64];
    char* password;
    int intentos = definir_intentos();

    printf("=========================================\n");
    printf(">>> INICIO DE SESION DEL AYUNTAMIENTO <<<\n");
    printf("=========================================\n");

    while (intentos > 0) {
        printf("\nUsuario: ");
        scanf("%63s", usuario);
        limpiarBuffer();
        password = capturar_contrasena();
     if (password == NULL) {
            printf("[ERROR] Error de memoria.\n");
            
            return 0;
        }

        char sql[256];
        snprintf(sql, sizeof(sql),
            "SELECT COUNT(*) FROM Admin "
            "WHERE nombre_usuario='%s' AND password='%s' AND activo=1;",
            usuario, password);

        int encontrado = 0;
        sqlite3_exec(db, sql, callback_login, &encontrado, NULL);
        char *usr = log_get_usuario();
        log_escribir("Ha buscado en la base de datos");
        free(password);

        if (encontrado) {
            printf("\n[OK] Bienvenido, %s!\n", usuario);
            log_set_usuario(usuario);
            log_escribir("Inicio de sesion exitoso");
            return 1;
        } else {
            intentos--;
            if (intentos > 0)
                printf("[ERROR] Credenciales incorrectas. Intentos restantes: %d\n", intentos);
                char msg[47]; //caracteres justos contados para fallos de 3 para abajp
                snprintf(msg, sizeof(msg), "Login fallido - intentos restantes: %d", intentos); //Es la unica manera que he encontrado de guardar un char escrita con algo de otra variable
                log_login_escribir(usuario,msg);
        }
    }

    printf("\n[ERROR] Demasiados intentos fallidos. Cerrando programa.\n");
    log_escribir("Acceso bloqueado por exceso de intentos fallidos");

    return 0;
}