#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "../include/cliente.h"

extern "C" {
#include "../include/funciones.h"
}

#define MAXDATASIZE 4096

using namespace cliente;

int g_cliente_socket = -1;
static char g_nombre_sesion[64] = "";

static void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Implementación del Constructor
Cliente::Cliente(int id, int edad, const char* nombre_cliente, const char* contrasena, const char* licencias) {
    this->id = id;
    this->edad = edad;
    this->nombre_cliente = (char*)malloc(strlen(nombre_cliente) + 1);
    strcpy(this->nombre_cliente, nombre_cliente);
    this->contrasena = (char*)malloc(strlen(contrasena) + 1);
    strcpy(this->contrasena, contrasena);
    this->licencias = (char*)malloc(strlen(licencias) + 1);
    strcpy(this->licencias, licencias);
}

// Implementación del Constructor Copia
Cliente::Cliente(const Cliente& otro) {
    this->id = otro.id;
    this->edad = otro.edad;
    this->nombre_cliente = (char*)malloc(strlen(otro.nombre_cliente) + 1);
    strcpy(this->nombre_cliente, otro.nombre_cliente);
    this->contrasena = (char*)malloc(strlen(otro.contrasena) + 1);
    strcpy(this->contrasena, otro.contrasena);
    this->licencias = (char*)malloc(strlen(otro.licencias) + 1);
    strcpy(this->licencias, otro.licencias);
}

// Implementación del Destructor
Cliente::~Cliente() {
    free(nombre_cliente);
    free(contrasena);
    free(licencias);
}

// Implementación de Getters
int Cliente::getId() const { return id; }
int Cliente::getEdad() const { return edad; }
const char* Cliente::getNombreCliente() const { return nombre_cliente; }
const char* Cliente::getContrasena() const { return contrasena; }
const char* Cliente::getLicencias() const { return licencias; }

extern "C" int cliente_conectar(const char *ip, const char *port)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    printf("\n=== INICIANDO CONEXIÓN CON SOCKET ===\n");

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(ip, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        printf("=== ERROR: No se pudo resolver dirección ===\n\n");
        return -1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
        printf("Intentando conectarse a %s:%s...\n", s, port);

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("connect");
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "=== ERROR: No se pudo conectar al servidor ===\n\n");
        freeaddrinfo(servinfo);
        return -1;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("✓ Conectado al servidor en %s:%s\n", s, port);
    printf("=== CONEXIÓN ESTABLECIDA ===\n\n");

    freeaddrinfo(servinfo);

    return sockfd;
}

extern "C" int cliente_enviar_recibir(int sockfd, const char *comando, char *respuesta, int max_size)
{
    if (sockfd == -1) {
        fprintf(stderr, "Error: socket no válido\n");
        return -1;
    }

    // Enviar comando completo (bucle por si send() no envía todo de golpe)
    size_t total = strlen(comando);
    size_t enviados = 0;
    while (enviados < total) {
        ssize_t n = send(sockfd, comando + enviados, total - enviados, 0);
        if (n == -1) { perror("send"); return -1; }
        enviados += (size_t)n;
    }

    // Recibir acumulando hasta encontrar el terminador \nEND
    int total_recibido = 0;
    while (total_recibido < max_size - 1) {
        int n = recv(sockfd, respuesta + total_recibido, max_size - 1 - total_recibido, 0);
        if (n == -1) { perror("recv"); return -1; }
        if (n == 0)  { printf("Servidor cerró la conexión\n"); return 0; }
        total_recibido += n;
        respuesta[total_recibido] = '\0';
        if (strstr(respuesta, "\nEND") != NULL) break;
    }

    // Eliminar el terminador antes de devolver al llamador
    char *end_marker = strstr(respuesta, "\nEND");
    if (end_marker) *end_marker = '\0';

    return total_recibido;
}

extern "C" void cliente_cerrar(int sockfd)
{
    if (sockfd != -1) {
        close(sockfd);
        printf("Conexión con servidor cerrada\n");
    }
}

extern "C" void cliente_set_socket(int sockfd) {
    g_cliente_socket = sockfd;
}

static int leer_max_intentos() {
    int max = 3;
    FILE *f = fopen("server.conf", "r");
    if (!f) return max;
    char linea[256], clave[128], valor[128];
    while (fgets(linea, sizeof(linea), f)) {
        if (linea[0] == '#' || linea[0] == '\n') continue;
        if (sscanf(linea, "%127[^=]=%127s", clave, valor) == 2) {
            if (strcmp(clave, "max_intentos") == 0) { max = atoi(valor); break; }
        }
    }
    fclose(f);
    return max;
}

static int g_intentos_fallidos = 0;

static int intentos_disponibles() {
    int max = leer_max_intentos();
    int restantes = max - g_intentos_fallidos;
    if (restantes <= 0) return 0;
    if (restantes < max)
        printf("[!] Intentos restantes: %d\n", restantes);
    return 1;
}

int cliente_intentos_agotados() {
    return g_intentos_fallidos >= leer_max_intentos();
}

extern "C" int cliente_login() {
    char usuario[64], password[64];
    char comando[256], respuesta[256];

    if (g_cliente_socket == -1) {
        printf("[ERROR] Socket no establecido. Debe conectarse al servidor primero.\n");
        return 0;
    }

    if (!intentos_disponibles()) return 0;

    printf("\n--- INICIAR SESION ---\n");
    printf("Usuario (nombre): ");
    fflush(stdout);
    scanf("%63s", usuario);
    limpiarBuffer();
    
    char *temp_password = capturar_contrasena();
    if (temp_password == NULL) {
        printf("[ERROR] Error al capturar contraseña.\n");
        return 0;
    }
    strncpy(password, temp_password, sizeof(password) - 1);
    password[sizeof(password) - 1] = '\0';
    free(temp_password);

    snprintf(comando, sizeof(comando), "LOGIN|%s|%s\n", usuario, password);

    if (cliente_enviar_recibir(g_cliente_socket, comando, respuesta, sizeof(respuesta)) > 0) {
        if (strncmp(respuesta, "OK", 2) == 0) {
            printf("[OK] Bienvenido, %s!\n", usuario);
            strncpy(g_nombre_sesion, usuario, sizeof(g_nombre_sesion) - 1);
            g_nombre_sesion[sizeof(g_nombre_sesion) - 1] = '\0';
            return 1;
        } else if (strncmp(respuesta, "USER_NOT_FOUND", 14) == 0) {
            g_intentos_fallidos++;
            printf("[ERROR] Usuario '%s' no encontrado.\n", usuario);
            return 0;
        } else {
            g_intentos_fallidos++;
            printf("[ERROR] Contrasena incorrecta.\n");
            return 0;
        }
    } else {
        printf("[ERROR] Fallo en la comunicación con el servidor.\n");
        return 0;
    }
}


static void invertir_recursivo(char *cad, int inicio, int fin) {
    if (inicio >= fin) return;                 // caso base
    char tmp = cad[inicio];
    cad[inicio] = cad[fin];
    cad[fin] = tmp;
    invertir_recursivo(cad, inicio + 1, fin - 1);  // llamada recursiva
}

extern "C" void cliente_registrar_nuevo() {
    char dni[32], usuario[64], password[64], password2[64];
    char comando[256], respuesta[256];

    if (g_cliente_socket == -1) {
        printf("[ERROR] Socket no establecido. Debe conectarse al servidor primero.\n");
        return;
    }

    if (!intentos_disponibles()) return;

    printf("\n--- REGISTRO DE NUEVO USUARIO ---\n");
    do {
        printf("DNI: "); scanf("%31s", dni); limpiarBuffer();
    } while (!dni_es_valido(dni));

printf("Usuario: "); scanf("%63s", usuario); limpiarBuffer();

    // --- Sugerencia de contrasena generada por algoritmo recursivo (nombre al reves) ---
    char sugerida[64];
    strncpy(sugerida, usuario, sizeof(sugerida) - 1);
    sugerida[sizeof(sugerida) - 1] = '\0';
    invertir_recursivo(sugerida, 0, (int)strlen(sugerida) - 1);

    printf("\nHemos generado esta contrasena a partir de tu nombre, usando nuestro algoritmo super avanzado(Vesga's inversor©®): %s\n", sugerida);
    printf("Quieres usar la contrasena creada por nuestro algoritmo?\n");
    printf("  1. Si, usar la sugerida\n");
    printf("  0. No, escribir la mia\n");
    printf("Seleccion: ");
    int usar_sugerida = obtener_entero_validado(0, 1);

    if (usar_sugerida == 1) {
        strncpy(password, sugerida, sizeof(password) - 1);
        password[sizeof(password) - 1] = '\0';
        printf("[OK] Usaras la contrasena sugerida.\n");
    } else {
        int pass_ok = 0;
        while (!pass_ok) {
            char *tmp1 = capturar_contrasena();
            if (!tmp1) { printf("[ERROR] Error al capturar contrasena.\n"); return; }
            strncpy(password, tmp1, sizeof(password) - 1);
            password[sizeof(password) - 1] = '\0';
            free(tmp1);

            printf("Repite la contrasena: ");
            char *tmp2 = capturar_contrasena();
            if (!tmp2) { printf("[ERROR] Error al capturar contrasena.\n"); return; }
            strncpy(password2, tmp2, sizeof(password2) - 1);
            password2[sizeof(password2) - 1] = '\0';
            free(tmp2);

            if (strcmp(password, password2) == 0) {
                pass_ok = 1;
            } else {
                g_intentos_fallidos++;
                printf("[ERROR] Las contrasenas no coinciden.");
                if (!intentos_disponibles()) return;
                printf(" Intentalo de nuevo.\n");
            }
        }
    }

    snprintf(comando, sizeof(comando), "REGISTER_CLIENTE|%s|%s|%s\n", dni, usuario, password);

    if (cliente_enviar_recibir(g_cliente_socket, comando, respuesta, sizeof(respuesta)) > 0) {
        if (strncmp(respuesta, "OK", 2) == 0) {
            printf("[OK] Cuenta creada correctamente. Ya puedes iniciar sesion.\n");
        } else {
            g_intentos_fallidos++;
            printf("[ERROR] %s\n", respuesta );
            intentos_disponibles();
        }
    } else {
        printf("[ERROR] Fallo en la comunicación con el servidor.\n");
    }
}

extern "C" const char* cliente_get_nombre_sesion(void) {
    return g_nombre_sesion;
}

extern "C" void cliente_cambiar_password(int sockfd) {
    char pass_actual[64], nueva_pass[64], confirmar[64];
    char comando[256], respuesta[256];

    if (sockfd == -1) {
        printf("[ERROR] Socket no valido.\n");
        return;
    }

    printf("\n--- CAMBIAR CONTRASENA ---\n");
    printf("Contrasena actual:\n");
    char *tmp = capturar_contrasena();
    if (!tmp) { printf("[ERROR] Error al capturar contrasena.\n"); return; }
    strncpy(pass_actual, tmp, sizeof(pass_actual) - 1);
    pass_actual[sizeof(pass_actual) - 1] = '\0';
    free(tmp);

    int ok = 0;
    while (!ok) {
        printf("Nueva contrasena:\n");
        tmp = capturar_contrasena();
        if (!tmp) { printf("[ERROR] Error al capturar contrasena.\n"); return; }
        strncpy(nueva_pass, tmp, sizeof(nueva_pass) - 1);
        nueva_pass[sizeof(nueva_pass) - 1] = '\0';
        free(tmp);

        printf("Confirmar nueva contrasena:\n");
        tmp = capturar_contrasena();
        if (!tmp) { printf("[ERROR] Error al capturar contrasena.\n"); return; }
        strncpy(confirmar, tmp, sizeof(confirmar) - 1);
        confirmar[sizeof(confirmar) - 1] = '\0';
        free(tmp);

        if (strcmp(nueva_pass, confirmar) == 0) {
            ok = 1;
        } else {
            printf("[ERROR] Las contrasenas no coinciden. Intentalo de nuevo.\n");
        }
    }

    snprintf(comando, sizeof(comando), "CAMBIAR_PASSWORD|%s|%s\n", pass_actual, nueva_pass);
    if (cliente_enviar_recibir(sockfd, comando, respuesta, sizeof(respuesta)) > 0) {
        if (strncmp(respuesta, "OK|", 3) == 0)
            printf("[OK] %s\n", respuesta + 3);
        else if (strncmp(respuesta, "ERROR|", 6) == 0)
            printf("[ERROR] %s\n", respuesta + 6);
    } else {
        printf("[ERROR] Fallo de comunicacion con el servidor.\n");
    }
}
