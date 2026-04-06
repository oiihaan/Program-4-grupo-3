#include "../include/auth.h"
#include "../include/db.h"
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
    char password[64];
    int intentos = 3;

    printf("=========================================\n");
    printf(">>> INICIO DE SESION DEL AYUNTAMIENTO <<<\n");
    printf("=========================================\n");

    while (intentos > 0) {
        printf("\nUsuario: ");
        scanf("%63s", usuario);
        printf("Password: ");
        scanf("%63s", password);

        char sql[256];
        snprintf(sql, sizeof(sql),
            "SELECT COUNT(*) FROM Admin "
            "WHERE nombre_usuario='%s' AND password='%s' AND activo=1;",
            usuario, password);

        int encontrado = 0;
        sqlite3_exec(db, sql, callback_login, &encontrado, NULL);

        if (encontrado) {
            printf("\n[OK] Bienvenido, %s!\n", usuario);
            return 1;
        } else {
            intentos--;
            if (intentos > 0)
                printf("[ERROR] Credenciales incorrectas. Intentos restantes: %d\n", intentos);
        }
    }

    printf("\n[ERROR] Demasiados intentos fallidos. Cerrando programa.\n");
    return 0;
}