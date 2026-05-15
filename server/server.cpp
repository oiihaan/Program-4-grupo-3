/* server.cpp - Servidor TCP para gestión de clientes */
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
#include <sys/select.h>
#include <iostream>

extern "C" {
#include "../include/db.h"
#include "../include/auth.h"
#include "../include/funciones.h"
}

#include <sqlite3.h>

#define PORT "5555"
#define BACKLOG 5
#define MAXDATASIZE 1024

extern sqlite3 *db;

static void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) return &(((struct sockaddr_in*)sa)->sin_addr);
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int handle_login(const char *usuario, const char *password, char *respuesta) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT password, fecha_creacion FROM Cliente WHERE nombre_cliente=? AND activo=1;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, usuario, -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *hash_db = (const char *)sqlite3_column_text(stmt, 0);
            const char *fecha_db = (const char *)sqlite3_column_text(stmt, 1);

            char hash_calculado[65];
            auth_generar_hash(password, fecha_db, hash_calculado);

            if (strcmp(hash_db, hash_calculado) == 0) {
                snprintf(respuesta, 256, "OK|LOGIN_SUCCESS");
                sqlite3_finalize(stmt);
                return 1;
            }
        }
        sqlite3_finalize(stmt);
    }

    snprintf(respuesta, 256, "ERROR|Usuario o contraseña incorrectos");
    return 0;
}

int handle_login_no_existe(const char *usuario, char *respuesta) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT 1 FROM Cliente WHERE nombre_cliente=?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, usuario, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_ROW) {
            snprintf(respuesta, 256, "USER_NOT_FOUND");
            sqlite3_finalize(stmt);
            return 1;
        }
        sqlite3_finalize(stmt);
    }
    return 0;
}

int handle_register(const char *dni, const char *usuario, const char *password, char *respuesta) {
    sqlite3_stmt *stmt = NULL;
    char hash_final[65];
    char fecha_aux[32] = "";

    // 1: Insertar registro base
    const char *sql_ins = "INSERT INTO Cliente (dni, nombre_cliente, password) VALUES (?, ?, 'temp');";

    if (sqlite3_prepare_v2(db, sql_ins, -1, &stmt, NULL) != SQLITE_OK) {
        snprintf(respuesta, 256, "ERROR|Fallo SQL al crear cliente: %s", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_text(stmt, 1, dni, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, usuario, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        snprintf(respuesta, 256, "ERROR|No se pudo insertar cliente: %s", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_finalize(stmt);

    // 2: Recuperar fecha_creacion
    const char *sql_sel = "SELECT fecha_creacion FROM Cliente WHERE dni = ?;";
    if (sqlite3_prepare_v2(db, sql_sel, -1, &stmt, NULL) != SQLITE_OK) {
        snprintf(respuesta, 256, "ERROR|Fallo SQL al leer fecha de cliente: %s", sqlite3_errmsg(db));
        return 0;
    }
    sqlite3_bind_text(stmt, 1, dni, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        strcpy(fecha_aux, (const char*)sqlite3_column_text(stmt, 0));
    }
    sqlite3_finalize(stmt);

    if (strlen(fecha_aux) == 0) {
        snprintf(respuesta, 256, "ERROR|No se pudo recuperar la fecha de creación del cliente");
        return 0;
    }

    // 3: Generar hash y actualizar
    auth_generar_hash(password, fecha_aux, hash_final);

    const char *sql_upd = "UPDATE Cliente SET password = ? WHERE dni = ?;";
    if (sqlite3_prepare_v2(db, sql_upd, -1, &stmt, NULL) != SQLITE_OK) {
        snprintf(respuesta, 256, "ERROR|Fallo SQL al actualizar password: %s", sqlite3_errmsg(db));
        return 0;
    }
    sqlite3_bind_text(stmt, 1, hash_final, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, dni, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        snprintf(respuesta, 256, "ERROR|No se pudo guardar el hash: %s", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_finalize(stmt);

    snprintf(respuesta, 256, "OK|Cliente registrado exitosamente");
    return 1;
}

int main(void) {
    int sockfd = -1;
    struct addrinfo hints, *servinfo, *p;
    int rv, yes = 1;
    char addrstr[INET6_ADDRSTRLEN];

    // Inicializar BD
    if (!db_abrir("./ayuntamiento.db")) {
        fprintf(stderr, "Error abriendo base de datos\n");
        return 1;
    }
    db_crear_tablas();

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), addrstr, sizeof addrstr);
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
            perror("setsockopt");
            close(sockfd);
            continue;
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("bind");
            close(sockfd);
            continue;
        }
        break;
    }
    freeaddrinfo(servinfo);
    if (p == NULL) { fprintf(stderr, "Fallo bind\n"); return 2; }
    if (listen(sockfd, BACKLOG) == -1) { perror("listen"); close(sockfd); return 3; }

    printf("Servidor: escuchando en puerto %s\n", PORT);

    while (1) {
        struct sockaddr_storage addr;
        socklen_t addrlen = sizeof addr;
        int cliente_fd = -1;

        printf("\nEsperando cliente...\n");
        cliente_fd = accept(sockfd, (struct sockaddr*)&addr, &addrlen);
        if (cliente_fd == -1) {
            perror("accept cliente");
            continue;
        }
        inet_ntop(addr.ss_family, get_in_addr((struct sockaddr*)&addr), addrstr, sizeof addrstr);
        printf("Cliente conectado desde %s\n", addrstr);

        char buffer[MAXDATASIZE];
        char respuesta[256];
        memset(buffer, 0, MAXDATASIZE);
        int recibido;

        while (1) {
            memset(buffer, 0, MAXDATASIZE);
            memset(respuesta, 0, 256);
            recibido = recv(cliente_fd, buffer, MAXDATASIZE - 1, 0);

            if (recibido == -1) {
                perror("recv cliente");
                break;
            } else if (recibido == 0) {
                printf("Cliente cerró la conexión\n");
                break;
            } else {
                buffer[recibido] = '\0';
                buffer[strcspn(buffer, "\r\n")] = '\0';
                printf("Comando recibido: %s\n", buffer);

                // Parse comando
                char cmd_copia[MAXDATASIZE];
                strcpy(cmd_copia, buffer);
                char *cmd = strtok(cmd_copia, "|");

                if (cmd == NULL || cmd[0] == '\0') {
                    snprintf(respuesta, 256, "ERROR|Comando vacío");
                    if (send(cliente_fd, respuesta, strlen(respuesta), 0) == -1) {
                        perror("send");
                        break;
                    }
                    continue;
                }

                if (strcmp(cmd, "LOGIN") == 0) {
                    char *usuario = strtok(NULL, "|");
                    char *password = strtok(NULL, "|");

                    if (usuario && password) {
                        char usuario_copia[64], password_copia[64];
                        strcpy(usuario_copia, usuario);
                        strcpy(password_copia, password);

                        // Primero verificar si existe el usuario
                        if (handle_login_no_existe(usuario_copia, respuesta)) {
                            // Usuario no existe
                            strcpy(respuesta, "USER_NOT_FOUND");
                        } else {
                            // Usuario existe, validar password
                            handle_login(usuario_copia, password_copia, respuesta);
                        }
                    } else {
                        snprintf(respuesta, 256, "ERROR|Comando LOGIN inválido");
                    }
                }
                else if (strcmp(cmd, "REGISTER_CLIENTE") == 0) {
                    char *dni = strtok(NULL, "|");
                    char *usuario = strtok(NULL, "|");
                    char *password = strtok(NULL, "|");

                    if (dni && usuario && password) {
                        char dni_copia[32], usuario_copia[64], password_copia[64];
                        strcpy(dni_copia, dni);
                        strcpy(usuario_copia, usuario);
                        strcpy(password_copia, password);

                        printf("[SERVER] Registrando: DNI=%s, Usuario=%s\n", dni_copia, usuario_copia);
                        handle_register(dni_copia, usuario_copia, password_copia, respuesta);
                        printf("[SERVER] Respuesta: %s\n", respuesta);
                    } else {
                        snprintf(respuesta, 256, "ERROR|Comando REGISTER_CLIENTE inválido");
                    }
                }
                else if (strcmp(cmd, "exit") == 0) {
                    printf("Cliente solicitó cerrar la conexión\n");
                    break;
                }
                else {
                    snprintf(respuesta, 256, "ERROR|Comando desconocido");
                }

                // Enviar respuesta
                if (send(cliente_fd, respuesta, strlen(respuesta), 0) == -1) {
                    perror("send");
                    break;
                }
            }
        }

        close(cliente_fd);
        printf("Conexión cerrada\n");
    }

    db_cerrar();
    close(sockfd);
    return 0;
}
