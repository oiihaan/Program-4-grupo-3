#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern "C" {
#include "../include/funciones.h"
#include "../include/espacios.h"
#include "../include/noticias.h"
#include "../include/licencias.h"
#include "../include/config.h"
#include "../include/db.h"
#include "../include/auth.h"
#include "../include/log.h"
}

//INCLUDEs para cURL (lo de la API del tiempo)
#include <curl/curl.h>

extern sqlite3 *db;

#define SERVER_PID_FILE ".server.pid"
static int g_server_port = 5555;

static int servidor_esta_activo() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return 0;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(g_server_port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    struct timeval tv;
    tv.tv_sec  = 1;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    int resultado = (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) ? 1 : 0;
    close(sock);
    return resultado;
}

static void servidor_arrancar() {
    printf("[INFO] Arrancando servidor en segundo plano...\n");
    pid_t pid = fork();
    if (pid < 0) {
        printf("[ERROR] No se pudo hacer fork para arrancar el servidor.\n");
        return;
    }
    if (pid == 0) {
        execl("./build/servidor.exe", "servidor.exe", (char *)NULL);
        perror("[ERROR] execl servidor.exe");
        _exit(1);
    }
    FILE *f = fopen(SERVER_PID_FILE, "w");
    if (f) {
        fprintf(f, "%d\n", (int)pid);
        fclose(f);
    }
    printf("[OK] Servidor arrancado con PID=%d.\n", (int)pid);
    log_escribir("El servidor ha sido arrancado desde el panel de administracion");
    sleep(1);
}

static void servidor_detener() {
    FILE *f = fopen(SERVER_PID_FILE, "r");
    if (!f) {
        printf("[AVISO] No se encontro el archivo %s. Intentando detener por nombre...\n", SERVER_PID_FILE);
        system("pkill -x servidor.exe 2>/dev/null");
        return;
    }
    int pid = 0;
    fscanf(f, "%d", &pid);
    fclose(f);

    if (pid <= 0) {
        printf("[ERROR] PID invalido en %s.\n", SERVER_PID_FILE);
        return;
    }
    if (kill(pid, SIGTERM) == 0) {
        printf("[OK] Servidor detenido (PID=%d).\n", pid);
        log_escribir("El servidor ha sido detenido desde el panel de administracion");
    } else {
        printf("[AVISO] No se pudo detener el proceso PID=%d (quizas ya no existe).\n", pid);
    }
    remove(SERVER_PID_FILE);
}

int main() {
    // Inicializa para que no pete la app basicamente (Prepara RAM)
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Log de encendido del sistema
    log_escribir("El sistema se ha encendido correctamente");  

    // cargar configuración
    if (!config_cargar("./server.conf")) return 1;

    if (config.server_puerto[0] != '\0') {
        int port_cfg = atoi(config.server_puerto);
        if (port_cfg > 0 && port_cfg < 65536) {
            g_server_port = port_cfg;
        }
    }

    // abrir base de datos
    if (!db_abrir(config.db_ruta)) return 1;
    
    // tablas
    db_crear_tablas();

    // --- CORRECCIÓN: DECLARACIÓN DE VARIABLES ---
    sqlite3_stmt *stmt;
    int total_admins = 0;
    const char *sql_check = "SELECT COUNT(*) FROM Admin;";
    // --------------------------------------------

    printf("[DEBUG] Comprobando usuarios en la BD...\n");
    log_escribir("Ha buscado en la base de datos");


    if (sqlite3_prepare_v2(db, sql_check, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            total_admins = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    } else {
        printf("[ERROR SQL] %s\n", sqlite3_errmsg(db)); 
    }

    printf("[DEBUG] Total admins encontrados: %d\n", total_admins);

    // NUEVO: Si no hay nadie, registrar uno
    if (total_admins == 0) {
        admin_registrar_nuevo();
        db_insertar_datos_prueba();
    }

    // Seed de publicaciones: si la tabla esta vacia, insertar datos de prueba
    int total_publicaciones = 0;
    const char *sql_check_pub = "SELECT COUNT(*) FROM Publicacion;";
    if (sqlite3_prepare_v2(db, sql_check_pub, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW)
            total_publicaciones = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    if (total_publicaciones == 0) {
        db_insertar_publicaciones_prueba();
    }

    // 4. Login
    if (!auth_login()) {
        log_escribir("El sistema se ha apagado\n");
        curl_global_cleanup();
        db_cerrar();
        return 0;
    }

    time_t inicio_sesion = time(NULL);

    // 5. Menú principal
    int opcion;
    do {
        printf("\n*** MENU PRINCIPAL DEL ADMINISTRADOR ***\n");
        printf("1. Gestion de espacios y sus reservas\n");
        printf("2. Gestion de noticias\n");
        printf("3. Gestion de licencias\n");
        printf("4. Configuracion\n");
        printf("5. Administrar servidor\n");
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
            case 5: {
                    int sub;
                    do {
                        int activo = servidor_esta_activo();

                        printf("\n--- ADMINISTRAR SERVIDOR ---\n");
                        if (activo) {
                            printf("[ESTADO] Servidor ENCENDIDO (puerto %d)\n", g_server_port);
                            printf("1. Ver logs de esta sesion\n");
                            printf("2. Apagar el servidor\n");
                        } else {
                            printf("[ESTADO] Servidor APAGADO\n");
                            printf("1. Encender el servidor\n");
                        }
                        printf("0. Volver al menu principal\n");
                        printf("Seleccion: ");

                        if (scanf("%d", &sub) != 1) { limpiarBuffer(); sub = -1; continue; }
                        limpiarBuffer();

                        if (activo) {
                            if (sub == 1) {
                                printf("\n--- LOGS DESDE EL INICIO DE SESION ---\n");
                                log_mostrar_desde(inicio_sesion);
                            } else if (sub == 2) {
                                servidor_detener();
                            }
                        } else {
                            if (sub == 1) {
                                servidor_arrancar();
                            }
                        }
                    } while (sub != 0);
                    break;
                }
            case 0:
                 printf("\n[INFO] Cerrando sesion. Hasta pronto!\n");
                 log_escribir("Ha cerrado la sesion");
                 log_set_usuario("Sistema");
                 break;
            default: printf("\n[ERROR] Opcion no valida.\n"); break;
        }
    } while (opcion != 0);

    log_escribir("El sistema se ha apagado\n");
    curl_global_cleanup();
    db_cerrar();
    return 0;
}
