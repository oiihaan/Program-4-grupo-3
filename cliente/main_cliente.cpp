#include <stdio.h>
#include "../include/cliente.h"

int main() {
    printf("=== CLIENTE CIUDADANO ===\n");
    printf("Conectando al servidor...\n");

    int sock = cliente_conectar("127.0.0.1", "5555");
    if (sock == -1) {
        printf("[ERROR] No se pudo conectar al servidor.\n");
        printf("Asegurate de que el servidor esta corriendo.\n");
        return 1;
    }

    printf("[OK] Conectado al servidor.\n");

    // Por ahora solo cerramos
    cliente_cerrar(sock);
    return 0;
}