#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/cliente.h"
#include "../include/server.h"

extern "C" {
#include "../include/funciones.h"
}

#define MAXDATASIZE 4096

// ─── HELPERS DE RESPUESTA ─────────────────────────────────────────────────────

// Imprime línea a línea una respuesta con formato "OK\nreg1\nreg2\nEND"
// Devuelve el número de registros mostrados, o -1 si fue un error
static int mostrar_respuesta_lista(const char *respuesta, const char *cabecera) {
    if (strncmp(respuesta, "ERROR|", 6) == 0) {
        printf("[ERROR] %s\n", respuesta + 6);
        return -1;
    }
    if (strncmp(respuesta, "OK", 2) != 0) {
        printf("[ERROR] Respuesta inesperada del servidor.\n");
        return -1;
    }

    printf("\n%s\n", cabecera);

    char copia[MAXDATASIZE];
    strncpy(copia, respuesta, MAXDATASIZE - 1);

    char *linea = strtok(copia, "\n");
    int total = 0;
    while ((linea = strtok(NULL, "\n")) != NULL) {
        if (strcmp(linea, "END") == 0) break;
        printf("  %s\n", linea);
        total++;
    }
    if (total == 0) printf("  (sin resultados)\n");
    return total;
}

static void cargar_puerto_servidor(char *dest, size_t size) {
    if (size == 0) return;
    strncpy(dest, "5555", size - 1);
    dest[size - 1] = '\0';

    FILE *f = fopen("server.conf", "r");
    if (!f) return;

    char linea[256];
    while (fgets(linea, sizeof(linea), f)) {
        if (linea[0] == '#' || linea[0] == '\n') continue;
        char clave[128], valor[128];
        if (sscanf(linea, "%127[^=]=%127s", clave, valor) == 2) {
            if (strcmp(clave, "server_puerto") == 0) {
                strncpy(dest, valor, size - 1);
                dest[size - 1] = '\0';
                break;
            }
        }
    }
    fclose(f);
}

// ─── ESPACIOS ─────────────────────────────────────────────────────────────────

static void mostrar_espacios(int sock) {
    char respuesta[MAXDATASIZE];
    cliente_enviar_recibir(sock, "LISTAR_ESPACIOS\n", respuesta, sizeof(respuesta));

    if (strncmp(respuesta, "ERROR|", 6) == 0) {
        printf("[ERROR] %s\n", respuesta + 6);
        return;
    }

    printf("\n--- ESPACIOS DISPONIBLES ---\n");
    printf("  %-4s %-25s %-10s %-12s\n", "ID", "Nombre", "Capacidad", "Precio/h");
    printf("  %s\n", "------------------------------------------------------------");

    char copia[MAXDATASIZE];
    strncpy(copia, respuesta, MAXDATASIZE - 1);
    char *linea = strtok(copia, "\n");
    int total = 0;
    while ((linea = strtok(NULL, "\n")) != NULL) {
        if (strcmp(linea, "END") == 0) break;
        int id, cap;
        char nombre[128];
        float precio;
        if (sscanf(linea, "%d;%127[^;];%d;%f", &id, nombre, &cap, &precio) == 4) {
            printf("  %-4d %-25s %-10d %.2f€\n", id, nombre, cap, precio);
            total++;
        }
    }
    if (total == 0) printf("  (no hay espacios disponibles)\n");
}

static void submenu_espacios(int sock) {
    mostrar_espacios(sock);
}

// ─── RESERVAS ─────────────────────────────────────────────────────────────────

static void ver_mis_reservas(int sock) {
    char respuesta[MAXDATASIZE];
    cliente_enviar_recibir(sock, "MIS_RESERVAS\n", respuesta, sizeof(respuesta));

    if (strncmp(respuesta, "ERROR|", 6) == 0) {
        printf("[ERROR] %s\n", respuesta + 6);
        return;
    }

    printf("\n--- MIS RESERVAS ---\n");
    printf("  %-4s %-20s %-12s %-6s %-6s %-8s %-10s\n",
           "ID", "Espacio", "Fecha", "Entr.", "Sal.", "Pers.", "Estado");
    printf("  %s\n", "------------------------------------------------------------------------");

    char copia[MAXDATASIZE];
    strncpy(copia, respuesta, MAXDATASIZE - 1);
    char *linea = strtok(copia, "\n");
    int total = 0;
    while ((linea = strtok(NULL, "\n")) != NULL) {
        if (strcmp(linea, "END") == 0) break;
        int id, personas;
        char espacio[64], fecha[16], inicio[8], fin[8], estado[12];
        if (sscanf(linea, "%d;%63[^;];%15[^;];%7[^;];%7[^;];%d;%11s",
                   &id, espacio, fecha, inicio, fin, &personas, estado) == 7) {
            printf("  %-4d %-20s %-12s %-6s %-6s %-8d %-10s\n",
                   id, espacio, fecha, inicio, fin, personas, estado);
            total++;
        }
    }
    if (total == 0) printf("  (no tienes reservas)\n");
}

static void crear_reserva(int sock) {
    // Primero mostramos los espacios para que el usuario elija
    mostrar_espacios(sock);

    int id_espacio;
    char fecha[16], inicio[8], fin[8];
    int num_personas;

    printf("\n--- CREAR RESERVA ---\n");
    printf("ID del espacio: ");
    if (scanf("%d", &id_espacio) != 1) { limpiarBuffer(); return; }
    limpiarBuffer();

    do {
        printf("Fecha (YYYY-MM-DD): ");
        scanf(" %15[^\n]", fecha);
        limpiarBuffer();
    } while (!fecha_es_valida(fecha) || !fecha_es_hoy_o_posterior(fecha));

    printf("Hora de entrada (HH:MM): ");
    scanf(" %7[^\n]", inicio); limpiarBuffer();

    printf("Hora de salida  (HH:MM): ");
    scanf(" %7[^\n]", fin); limpiarBuffer();

    printf("Numero de personas: ");
    if (scanf("%d", &num_personas) != 1) { limpiarBuffer(); return; }
    limpiarBuffer();

    char comando[256], respuesta[MAXDATASIZE];
    snprintf(comando, sizeof(comando), "CREAR_RESERVA|%d|%s|%s|%s|%d\n",
             id_espacio, fecha, inicio, fin, num_personas);

    cliente_enviar_recibir(sock, comando, respuesta, sizeof(respuesta));

    if (strncmp(respuesta, "OK|", 3) == 0)
        printf("[OK] %s\n", respuesta + 3);
    else if (strncmp(respuesta, "ERROR|", 6) == 0)
        printf("[ERROR] %s\n", respuesta + 6);
}

static void cancelar_reserva(int sock) {
    ver_mis_reservas(sock);

    int id_reserva;
    printf("\nID de la reserva a cancelar (0 para volver): ");
    if (scanf("%d", &id_reserva) != 1 || id_reserva == 0) { limpiarBuffer(); return; }
    limpiarBuffer();

    char comando[64], respuesta[MAXDATASIZE];
    snprintf(comando, sizeof(comando), "CANCELAR_RESERVA|%d\n", id_reserva);

    cliente_enviar_recibir(sock, comando, respuesta, sizeof(respuesta));

    if (strncmp(respuesta, "OK|", 3) == 0)
        printf("[OK] %s\n", respuesta + 3);
    else if (strncmp(respuesta, "ERROR|", 6) == 0)
        printf("[ERROR] %s\n", respuesta + 6);
}

static void submenu_reservas(int sock) {
    int opcion;
    do {
        server_verificar_estado();
        if (!server_get_estado()) {
            printf("\n[AVISO] El servidor se ha apagado. Cerrando cliente...\n");
            cliente_cerrar(sock);
            exit(0);
        }

        printf("\n--- RESERVAS ---\n");
        printf("1. Ver mis reservas\n");
        printf("2. Crear reserva\n");
        printf("3. Cancelar reserva\n");
        printf("0. Volver\n");
        printf("Seleccion: ");

        if (scanf("%d", &opcion) != 1) { limpiarBuffer(); opcion = -1; continue; }
        limpiarBuffer();

        switch (opcion) {
            case 1: ver_mis_reservas(sock); break;
            case 2: crear_reserva(sock);    break;
            case 3: cancelar_reserva(sock); break;
            case 0: break;
            default: printf("[!] Opcion invalida.\n");
        }
    } while (opcion != 0);
}

// ─── NOTICIAS ─────────────────────────────────────────────────────────────────

static void mostrar_noticias(int sock, const char *categoria) {
    char comando[64], respuesta[MAXDATASIZE];

    if (categoria)
        snprintf(comando, sizeof(comando), "LISTAR_NOTICIAS|%s\n", categoria);
    else
        snprintf(comando, sizeof(comando), "LISTAR_NOTICIAS\n");

    cliente_enviar_recibir(sock, comando, respuesta, sizeof(respuesta));

    if (strncmp(respuesta, "ERROR|", 6) == 0) {
        printf("[ERROR] %s\n", respuesta + 6);
        return;
    }

    const char *titulo = categoria ? categoria : "TODAS";
    printf("\n--- NOTICIAS: %s ---\n", titulo);
    printf("  %-4s %-12s %-32s %-12s %-12s\n", "ID", "Categoria", "Titulo", "Fecha", "Usuario");
    printf("  %s\n", "---------------------------------------------------------------------------------");

    char copia[MAXDATASIZE];
    strncpy(copia, respuesta, MAXDATASIZE - 1);
    char *linea = strtok(copia, "\n");
    int total = 0;
    while ((linea = strtok(NULL, "\n")) != NULL) {
        if (strcmp(linea, "END") == 0) break;
        /* Parsear manualmente para soportar enlace vacío (;;) */
        int id = 0;
        char cat[32]="", tit[128]="", enlace[256]="", fecha[32]="", usuario[64]="-";
        char *p = linea;
        char *sep;
        /* campo 1: id */
        id = (int)strtol(p, &sep, 10); if (sep == p || *sep != ';') continue; p = sep + 1;
        /* campo 2: categoria */
        sep = strchr(p, ';'); if (!sep) continue; *sep = '\0'; strncpy(cat, p, 31); p = sep + 1;
        /* campo 3: titulo */
        sep = strchr(p, ';'); if (!sep) continue; *sep = '\0'; strncpy(tit, p, 127); p = sep + 1;
        /* campo 4: enlace (puede ser vacío) */
        sep = strchr(p, ';'); if (!sep) continue; *sep = '\0'; strncpy(enlace, p, 255); p = sep + 1;
        /* campo 5: fecha */
        sep = strchr(p, ';'); 
        if (sep) { *sep = '\0'; strncpy(fecha, p, 31); p = sep + 1;
            /* campo 6: usuario (opcional) */
            strncpy(usuario, p, 63); }
        else { strncpy(fecha, p, 31); }
        if (fecha[0] != '\0') {
            printf("  %-4d %-12s %-32s %-12s %-12s\n", id, cat, tit, fecha, usuario);
            if (enlace[0] != '\0') printf("       Enlace: %s\n", enlace);
            total++;
        }
    }
    if (total == 0) printf("  (no hay noticias)\n");
}

static void submenu_noticias(int sock, cliente::Usuario *sesion_usr) {
    int opcion;
    do {
        server_verificar_estado();
        if (!server_get_estado()) {
            printf("\n[AVISO] El servidor se ha apagado. Cerrando cliente...\n");
            cliente_cerrar(sock);
            exit(0);
        }

        printf("\n--- NOTICIAS ---\n");
        printf("1. Todas las noticias\n");
        printf("2. Deportes\n");
        if (sesion_usr && sesion_usr->puedeVerPolitica())
            printf("3. Politica\n");
        else
            printf("3. Politica [solo mayores de edad]\n");
        printf("4. El tiempo\n");
        printf("0. Volver\n");
        printf("Seleccion: ");

        if (scanf("%d", &opcion) != 1) { limpiarBuffer(); opcion = -1; continue; }
        limpiarBuffer();

        switch (opcion) {
            case 1: mostrar_noticias(sock, NULL);       break;
            case 2: mostrar_noticias(sock, "Deportes"); break;
            case 3:
                if (sesion_usr && sesion_usr->puedeVerPolitica())
                    mostrar_noticias(sock, "Politica");
                else
                    printf("[AVISO] Las noticias de Politica son solo para mayores de edad.\n");
                break;
            case 4: mostrarTiempo();                    break;
            case 0: break;
            default: printf("[!] Opcion invalida.\n");
        }
    } while (opcion != 0);
}

// ─── LICENCIAS ────────────────────────────────────────────────────────────────

static void ver_tipos_licencia(int sock) {
    char respuesta[MAXDATASIZE];
    cliente_enviar_recibir(sock, "LISTAR_TIPOS_LICENCIA\n", respuesta, sizeof(respuesta));

    if (strncmp(respuesta, "ERROR|", 6) == 0) {
        printf("[ERROR] %s\n", respuesta + 6);
        return;
    }

    printf("\n--- TIPOS DE LICENCIA DISPONIBLES ---\n");

    char copia[MAXDATASIZE];
    strncpy(copia, respuesta, MAXDATASIZE - 1);
    char *linea = strtok(copia, "\n");
    int total = 0;
    while ((linea = strtok(NULL, "\n")) != NULL) {
        if (strcmp(linea, "END") == 0) break;
        int id;
        char nombre[64], desc[128], req[128];
        if (sscanf(linea, "%d;%63[^;];%127[^;];%127[^\n]", &id, nombre, desc, req) >= 3) {
            printf("  [%d] %s\n", id, nombre);
            printf("       Descripcion: %s\n", desc);
            printf("       Requisitos:  %s\n", req);
            printf("       ---\n");
            total++;
        }
    }
    if (total == 0) printf("  (no hay tipos de licencia disponibles)\n");
}

static void solicitar_licencia(int sock, cliente::Usuario *sesion_usr) {
    ver_tipos_licencia(sock);

    int id_tipo;
    printf("\nID del tipo de licencia (0 para volver): ");
    if (scanf("%d", &id_tipo) != 1 || id_tipo == 0) { limpiarBuffer(); return; }
    limpiarBuffer();

    if (id_tipo == 5 && sesion_usr && !sesion_usr->puedeVerPolitica()) {
        printf("[AVISO] La Licencia de Armas es solo para mayores de edad.\n");
        return;
    }

    char comando[64], respuesta[MAXDATASIZE];
    snprintf(comando, sizeof(comando), "SOLICITAR_LICENCIA|%d\n", id_tipo);

    cliente_enviar_recibir(sock, comando, respuesta, sizeof(respuesta));

    if (strncmp(respuesta, "OK|", 3) == 0)
        printf("[OK] %s\n", respuesta + 3);
    else if (strncmp(respuesta, "ERROR|", 6) == 0)
        printf("[ERROR] %s\n", respuesta + 6);
}

static void ver_mis_licencias(int sock) {
    char respuesta[MAXDATASIZE];
    cliente_enviar_recibir(sock, "MIS_LICENCIAS\n", respuesta, sizeof(respuesta));

    if (strncmp(respuesta, "ERROR|", 6) == 0) {
        printf("[ERROR] %s\n", respuesta + 6);
        return;
    }

    printf("\n--- MIS LICENCIAS ---\n");
    printf("  %-4s %-20s %-14s %-12s %-12s\n",
           "ID", "Tipo", "Estado", "Solicitud", "Expiracion");
    printf("  %s\n", "--------------------------------------------------------------");

    char copia[MAXDATASIZE];
    strncpy(copia, respuesta, MAXDATASIZE - 1);
    char *linea = strtok(copia, "\n");
    int total = 0;
    while ((linea = strtok(NULL, "\n")) != NULL) {
        if (strcmp(linea, "END") == 0) break;
        int id;
        char tipo[64], estado[16], f_sol[12], f_exp[12];
        if (sscanf(linea, "%d;%63[^;];%15[^;];%11[^;];%11s",
                   &id, tipo, estado, f_sol, f_exp) == 5) {
            printf("  %-4d %-20s %-14s %-12s %-12s\n",
                   id, tipo, estado, f_sol, f_exp);
            total++;
        }
    }
    if (total == 0) printf("  (no tienes licencias)\n");
}

static void submenu_licencias(int sock, cliente::Usuario *sesion) {
    int opcion;
    do {
        server_verificar_estado();
        if (!server_get_estado()) {
            printf("\n[AVISO] El servidor se ha apagado. Cerrando cliente...\n");
            cliente_cerrar(sock);
            exit(0);
        }

        printf("\n--- LICENCIAS ---\n");
        printf("1. Ver tipos de licencia disponibles\n");
        printf("2. Solicitar licencia\n");
        printf("3. Ver mis licencias\n");
        printf("0. Volver\n");
        printf("Seleccion: ");

        if (scanf("%d", &opcion) != 1) { limpiarBuffer(); opcion = -1; continue; }
        limpiarBuffer();

        switch (opcion) {
            case 1: ver_tipos_licencia(sock); break;
            case 2: solicitar_licencia(sock,sesion); break;
            case 3: ver_mis_licencias(sock);  break;
            case 0: break;
            default: printf("[!] Opcion invalida.\n");
        }
    } while (opcion != 0);
}

// ─── MAIN ─────────────────────────────────────────────────────────────────────

int main() {
    printf("=== AYUNTAMIENTO DE DONOSTIA - PORTAL CIUDADANO ===\n");
    printf("Conectando al servidor...\n");

    char puerto[16];
    cargar_puerto_servidor(puerto, sizeof(puerto));
    int port_num = atoi(puerto);
    server_configurar_desde_puerto(port_num);

    int sock = cliente_conectar("127.0.0.1", puerto);
    if (sock == -1) {
        printf("[ERROR] No se pudo conectar al servidor.\n");
        printf("Asegurate de que el servidor esta corriendo.\n");
        return 1;
    }
    printf("[OK] Conectado.\n");

    cliente_set_socket(sock);

    int autenticado = 0;
    cliente::Usuario *sesion = NULL;
    while (!autenticado) {
        server_verificar_estado();
        if (!server_get_estado()) {
            printf("\n[AVISO] El servidor se ha apagado. Cerrando cliente...\n");
            cliente_cerrar(sock);
            return 0;
        }

        int opcion_inicio;
        printf("\n=== INICIO ===\n");
        printf("1. Iniciar sesion\n");
        printf("2. Crear usuario\n");
        printf("0. Salir\n");
        printf("Seleccion: ");

        if (scanf("%d", &opcion_inicio) != 1) { limpiarBuffer(); continue; }
        limpiarBuffer();

        if (opcion_inicio == 1) {
            int es_mayor = 1;
            autenticado = cliente_login(&es_mayor);
            if (autenticado) {
                if (es_mayor)
                    sesion = new cliente::ClienteMayor(0, cliente_get_nombre_sesion(), "");
                else
                    sesion = new cliente::ClienteMenor(0, cliente_get_nombre_sesion(), "");
            }
            if (!autenticado && cliente_intentos_agotados()) {
                printf("\n[BLOQUEADO] Has superado el numero maximo de intentos. Adios.\n");
                cliente_cerrar(sock);
                return 1;
            }
        } else if (opcion_inicio == 2) {
            cliente_registrar_nuevo();
            if (cliente_intentos_agotados()) {
                printf("\n[BLOQUEADO] Has superado el numero maximo de intentos. Adios.\n");
                cliente_cerrar(sock);
                return 1;
            }
        } else if (opcion_inicio == 0) {
            cliente_cerrar(sock);
            return 0;
        } else {
            printf("[!] Opcion invalida.\n");
        }
    }

    int opcion;
    do {
        server_verificar_estado();
        if (!server_get_estado()) {
            printf("\n[AVISO] El servidor se ha apagado. Cerrando cliente...\n");
            break;
        }

        printf("\n=== MENU PRINCIPAL — %s ===\n", sesion ? sesion->getNombre() : "");
        printf("1. Ver espacios disponibles\n");
        printf("2. Mis reservas\n");
        printf("3. Noticias\n");
        printf("4. Licencias\n");
        printf("5. Mi cuenta\n");
        printf("0. Cerrar sesion\n");
        printf("Seleccion: ");

        if (scanf("%d", &opcion) != 1) { limpiarBuffer(); opcion = -1; continue; }
        limpiarBuffer();

        switch (opcion) {
            case 1: submenu_espacios(sock);  break;
            case 2: submenu_reservas(sock);  break;
            case 3: submenu_noticias(sock, sesion);  break;
            case 4: submenu_licencias(sock,sesion); break;
            case 5: {
                int sub_op;
                printf("\n--- MI CUENTA (%s) — %s ---\n", sesion ? sesion->getNombre() : "", sesion ? sesion->getTipo() : "");
                printf("1. Cambiar contrasena\n");
                printf("0. Volver\n");
                printf("Seleccion: ");
                if (scanf("%d", &sub_op) != 1) { limpiarBuffer(); break; }
                limpiarBuffer();
                if (sub_op == 1) cliente_cambiar_password(sock);
                break;
            }
            case 0: printf("\n[INFO] Cerrando sesion. Hasta pronto!\n"); break;
            default: printf("[!] Opcion invalida.\n");
        }
    } while (opcion != 0);

    if (sesion) { delete sesion; sesion = NULL; }
    cliente_cerrar(sock);
    return 0;
}
