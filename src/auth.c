#include "../include/auth.h"
#include "../include/db.h"
#include "../include/config.h"
#include "../include/funciones.h"
#include "../include/log.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int auth_login() {
    char usuario[64];
    char *password;
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

        // Cambio el sprintf por una sentencia preparada para evitarnos una sql inyection
        // Tambien quito el buffer de tamaño fijo para evitar buffer overflow
        sqlite3_stmt *stmt;
        const char *sql =
            "SELECT COUNT(*) FROM Admin "
            "WHERE nombre_usuario=? AND password=? AND activo=1;";

        int encontrado = 0;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, usuario,  -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW)
                encontrado = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
        }

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
            char msg[47];
            snprintf(msg, sizeof(msg), "Login fallido - intentos restantes: %d", intentos);
            log_login_escribir(usuario, msg);
        }
    }

    printf("\n[ERROR] Demasiados intentos fallidos. Cerrando programa.\n");
    log_escribir("Acceso bloqueado por exceso de intentos fallidos");
    return 0;
}