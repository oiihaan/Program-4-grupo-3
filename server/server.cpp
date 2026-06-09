/* server.cpp - Servidor TCP para gestión de clientes */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

extern "C" {
#ifndef CLIENT_MODE
#include "../include/db.h"
#include "../include/auth.h"
#include "../include/config.h"
#endif
#include "../include/funciones.h"
#include "../include/log.h"
}

#ifndef CLIENT_MODE
#include <sqlite3.h>
#endif
#include "../include/server.h"

#define PORT_FALLBACK "5555"
#define BACKLOG     10
#define MAXDATASIZE 4096

#ifndef CLIENT_MODE
extern sqlite3 *db;
#endif
using namespace server;

// ─── IMPLEMENTACIÓN CLASE SERVER ──────────────────────────────────────────

static Server *g_server = NULL;

Server::Server(int puerto) {
    this->puerto = puerto;
    this->estado = 0;
}
Server::~Server() {
}

int Server::getEstado() const {
    return estado;
}
void Server::setEstado(int estado) {
    this->estado = (estado == 1) ? 1 : 0;
}
int Server::getPuerto() const {
    return puerto;
}
void Server::setPuerto(int puerto) {
    this->puerto = puerto;
}

extern "C" {
    void server_configurar_desde_puerto(int puerto) {
        if (g_server == NULL) {
            g_server = new Server(puerto);
        } else {
            g_server->setPuerto(puerto);
        }
    }

    int server_get_puerto(void) {
        return (g_server != NULL) ? g_server->getPuerto() : atoi(PORT_FALLBACK);
    }

    void server_set_estado(int estado) {
        if (g_server != NULL) {
            g_server->setEstado(estado);
        }
    }

    int server_get_estado(void) {
        return (g_server != NULL) ? g_server->getEstado() : 0;
    }

    void server_verificar_estado(void) {
        int puerto = server_get_puerto();

        if (server_probar_puerto(puerto)) {
            if (g_server != NULL) g_server->setEstado(1);
            return;
        }

        const char *pid_file = SERVER_PID_FILE;
        FILE *f = fopen(pid_file, "r");
        if (f) {
            int pid = 0;
            fscanf(f, "%d", &pid);
            fclose(f);
            if (pid > 0 && kill(pid, 0) != 0) {
                remove(pid_file);
            }
        }

        if (g_server != NULL) g_server->setEstado(0);
    }

    int server_probar_puerto(int puerto) {
        if (puerto <= 0 || puerto >= 65536) return 0;

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return 0;

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port        = htons((uint16_t)puerto);

        int resultado = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
        close(sock);

        return (resultado == 0) ? 1 : 0;
    }
}



// ─── HELPERS ──────────────────────────────────────────────────────────────────

#ifndef CLIENT_MODE

static int hora_a_minutos(const char *hora) {
    int h = 0, m = 0;
    sscanf(hora, "%d:%d", &h, &m);
    return h * 60 + m;
}

static int hora_formato_valido(const char *hora) {
    if (!hora || strlen(hora) != 5 || hora[2] != ':') return 0;
    if (!isdigit((unsigned char)hora[0]) || !isdigit((unsigned char)hora[1]) ||
        !isdigit((unsigned char)hora[3]) || !isdigit((unsigned char)hora[4])) return 0;
    int h = 0, m = 0;
    if (sscanf(hora, "%2d:%2d", &h, &m) != 2) return 0;
    return (h >= 0 && h <= 23 && m >= 0 && m <= 59);
}

static int fecha_formato_valido(const char *fecha) {
    if (!fecha || strlen(fecha) != 10) return 0;
    if (fecha[4] != '-' || fecha[7] != '-') return 0;
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) continue;
        if (!isdigit((unsigned char)fecha[i])) return 0;
    }
    int anio = 0, mes = 0, dia = 0;
    if (sscanf(fecha, "%4d-%2d-%2d", &anio, &mes, &dia) != 3) return 0;
    if (mes < 1 || mes > 12 || dia < 1) return 0;
    int dias_mes[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int bisiesto = ((anio % 4 == 0 && anio % 100 != 0) || (anio % 400 == 0));
    if (bisiesto) dias_mes[1] = 29;
    return dia <= dias_mes[mes - 1];
}

static int hay_solapamiento(const char *i1, const char *f1, const char *i2, const char *f2) {
    return hora_a_minutos(i1) < hora_a_minutos(f2) &&
           hora_a_minutos(i2) < hora_a_minutos(f1);
}

// ─── AUTH ─────────────────────────────────────────────────────────────────────

static int handle_login_no_existe(const char *usuario) {
    sqlite3_stmt *stmt;
    int existe = 0;
    const char *sql = "SELECT 1 FROM Cliente WHERE nombre_cliente=?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, usuario, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) existe = 1;
        sqlite3_finalize(stmt);
    }
    return !existe;  // retorna 1 si NO existe
}

static int handle_login(const char *usuario, const char *password,
                        char *dni_out, char *respuesta) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT password, fecha_creacion, dni, fecha_nacimiento FROM Cliente "
                      "WHERE nombre_cliente=? AND activo=1;";
    int ok = 0;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, usuario, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *hash_db  = (const char *)sqlite3_column_text(stmt, 0);
            const char *fecha_db = (const char *)sqlite3_column_text(stmt, 1);
            const char *dni_db   = (const char *)sqlite3_column_text(stmt, 2);

            // Compatibilidad: cuentas creadas con bug previo guardaban "temp".
            // En ese caso, usar la contraseña tecleada para autocorregir el hash.
            if (hash_db && strcmp(hash_db, "temp") == 0) {
                char hash_migrado[65];
                auth_generar_hash(password, fecha_db, hash_migrado);

                sqlite3_stmt *stmt_upd = NULL;
                const char *sql_upd = "UPDATE Cliente SET password=? WHERE dni=?;";
                if (sqlite3_prepare_v2(db, sql_upd, -1, &stmt_upd, NULL) == SQLITE_OK) {
                    sqlite3_bind_text(stmt_upd, 1, hash_migrado, -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text(stmt_upd, 2, dni_db,       -1, SQLITE_TRANSIENT);
                    sqlite3_step(stmt_upd);
                    sqlite3_finalize(stmt_upd);
                    hash_db = hash_migrado;
                }
            }

            char hash_calc[65];
            auth_generar_hash(password, fecha_db, hash_calc);

            if (strcmp(hash_db, hash_calc) == 0) {
                const char *fn = (const char *)sqlite3_column_text(stmt, 3);
                int mayor = 1; /* por defecto mayor si no hay fecha */
                if (fn && fn[0]) {
                    int anio=0,mes=0,dia=0;
                    sscanf(fn, "%4d-%2d-%2d", &anio, &mes, &dia);
                    time_t ahora = time(NULL);
                    struct tm *hoy = localtime(&ahora);
                    int edad = (hoy->tm_year+1900) - anio;
                    if ((hoy->tm_mon+1) < mes || ((hoy->tm_mon+1)==mes && hoy->tm_mday < dia)) edad--;
                    mayor = (edad >= 18);
                }
                snprintf(respuesta, MAXDATASIZE, "OK|LOGIN_SUCCESS|%s", mayor ? "MAYOR" : "MENOR");
                if (dni_out) strncpy(dni_out, dni_db, 31);
                ok = 1;
            }
        }
        sqlite3_finalize(stmt);
    }
    if (!ok) snprintf(respuesta, MAXDATASIZE, "ERROR|Usuario o contrasena incorrectos");
    return ok;
}

static void handle_register(const char *dni, const char *usuario,
                            const char *password, const char *fecha_nac,
                            char *respuesta) {
    sqlite3_stmt *stmt = NULL;
    char hash_final[65], fecha_aux[32] = "";

    // Generar una fecha única y usarla tanto para hash como para INSERT
    const char *sql_now = "SELECT CURRENT_TIMESTAMP;";
    if (sqlite3_prepare_v2(db, sql_now, -1, &stmt, NULL) != SQLITE_OK) {
        snprintf(respuesta, MAXDATASIZE, "ERROR|Fallo SQL al obtener timestamp: %s", sqlite3_errmsg(db));
        return;
    }
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *ts = sqlite3_column_text(stmt, 0);
        if (ts) strncpy(fecha_aux, (const char*)ts, sizeof(fecha_aux) - 1);
    }
    sqlite3_finalize(stmt);

    if (!fecha_aux[0]) {
        snprintf(respuesta, MAXDATASIZE, "No se pudo generar fecha de creacion");
        return;
    }

    const char *sql_check_usr = "SELECT 1 FROM Cliente WHERE nombre_cliente=?;";
    if (sqlite3_prepare_v2(db, sql_check_usr, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, usuario, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            sqlite3_finalize(stmt);
            snprintf(respuesta, MAXDATASIZE, "El nombre de usuario '%s' ya esta en uso", usuario);
            return;
        }
        sqlite3_finalize(stmt);
    }

    const char *sql_check_dni = "SELECT 1 FROM Cliente WHERE dni=?;";
    if (sqlite3_prepare_v2(db, sql_check_dni, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, dni, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            sqlite3_finalize(stmt);
            snprintf(respuesta, MAXDATASIZE, "El DNI '%s' ya tiene una cuenta registrada", dni);
            return;
        }
        sqlite3_finalize(stmt);
    }

    auth_generar_hash(password, fecha_aux, hash_final);

    const char *sql_ins =
        "INSERT INTO Cliente (dni, nombre_cliente, password, fecha_nacimiento, activo, fecha_creacion) "
        "VALUES (?, ?, ?, ?, 1, ?);";
    if (sqlite3_prepare_v2(db, sql_ins, -1, &stmt, NULL) != SQLITE_OK) {
        snprintf(respuesta, MAXDATASIZE, "ERROR|Fallo SQL: %s", sqlite3_errmsg(db));
        return;
    }

    sqlite3_bind_text(stmt, 1, dni,        -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, usuario,    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, hash_final, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, fecha_nac ? fecha_nac : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, fecha_aux,  -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        const char *sql_err = sqlite3_errmsg(db);
        if (strstr(sql_err, "nombre_cliente")) {
            snprintf(respuesta, MAXDATASIZE, "ERROR|Ya existe una cuenta con ese nombre de usuario");
        } else if (strstr(sql_err, "dni")) {
            snprintf(respuesta, MAXDATASIZE, "ERROR|Ya existe una cuenta con ese DNI");
        } else {
            snprintf(respuesta, MAXDATASIZE, "ERROR|No se pudo completar el registro");
        }
        sqlite3_finalize(stmt);
        return;
    }

    sqlite3_finalize(stmt);

    snprintf(respuesta, MAXDATASIZE, "OK|Cliente registrado exitosamente");
}

// ─── ESPACIOS ─────────────────────────────────────────────────────────────

static void handle_listar_espacios(char *respuesta) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id_espacio, nombre, capacidad, precio_hora FROM Espacio WHERE activo=1;";
    char buf[MAXDATASIZE];
    int pos = snprintf(buf, sizeof(buf), "OK");

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW && pos < (int)sizeof(buf) - 100) {
            pos += snprintf(buf + pos, sizeof(buf) - pos, "\n%d;%s;%d;%.2f",
                sqlite3_column_int(stmt,    0),
                sqlite3_column_text(stmt,   1),
                sqlite3_column_int(stmt,    2),
                sqlite3_column_double(stmt, 3));
        }
        sqlite3_finalize(stmt);
    }
    snprintf(buf + pos, sizeof(buf) - pos, "\nEND");
    strncpy(respuesta, buf, MAXDATASIZE - 1);
}

// ─── RESERVAS ─────────────────────────────────────────────────────────────

static void handle_mis_reservas(const char *dni, char *respuesta) {
    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT r.id_reserva, e.nombre, r.fecha, r.franja_inicio, r.franja_fin, "
        "r.num_personas, r.cancelada "
        "FROM Reserva r JOIN Espacio e ON r.id_espacio=e.id_espacio "
        "WHERE r.dni_ciudadano=? ORDER BY r.fecha DESC;";
    char buf[MAXDATASIZE];
    int pos = snprintf(buf, sizeof(buf), "OK");

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, dni, -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW && pos < (int)sizeof(buf) - 150) {
            pos += snprintf(buf + pos, sizeof(buf) - pos, "\n%d;%s;%s;%s;%s;%d;%s",
                sqlite3_column_int(stmt,  0),
                sqlite3_column_text(stmt, 1),
                sqlite3_column_text(stmt, 2),
                sqlite3_column_text(stmt, 3),
                sqlite3_column_text(stmt, 4),
                sqlite3_column_int(stmt,  5),
                sqlite3_column_int(stmt,  6) ? "CANCELADA" : "ACTIVA");
        }
        sqlite3_finalize(stmt);
    }
    snprintf(buf + pos, sizeof(buf) - pos, "\nEND");
    strncpy(respuesta, buf, MAXDATASIZE - 1);
}

static void handle_crear_reserva(const char *dni, const char *id_esp_str,
                                  const char *fecha, const char *inicio,
                                  const char *fin, const char *personas_str,
                                  char *respuesta) {
    int id_espacio  = atoi(id_esp_str);
    int num_personas = atoi(personas_str);
    sqlite3_stmt *stmt;

    if (!fecha_formato_valido(fecha)) {
        snprintf(respuesta, MAXDATASIZE, "ERROR|Formato de fecha invalido (YYYY-MM-DD)");
        return;
    }
    if (!hora_formato_valido(inicio) || !hora_formato_valido(fin)) {
        snprintf(respuesta, MAXDATASIZE, "ERROR|Formato de hora invalido (HH:MM)");
        return;
    }

    // Verificar espacio activo y obtener capacidad
    int capacidad_max = 0;
    const char *sql_cap = "SELECT capacidad FROM Espacio WHERE id_espacio=? AND activo=1;";
    if (sqlite3_prepare_v2(db, sql_cap, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id_espacio);
        if (sqlite3_step(stmt) == SQLITE_ROW)
            capacidad_max = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    if (capacidad_max == 0) {
        snprintf(respuesta, MAXDATASIZE, "ERROR|El espacio no existe o no esta activo");
        return;
    }
    if (num_personas < 1 || num_personas > capacidad_max) {
        snprintf(respuesta, MAXDATASIZE, "ERROR|El espacio admite entre 1 y %d personas", capacidad_max);
        return;
    }
    if (hora_a_minutos(inicio) >= hora_a_minutos(fin)) {
        snprintf(respuesta, MAXDATASIZE, "ERROR|La hora de entrada debe ser anterior a la de salida");
        return;
    }

    // Verificar solapamiento con reservas existentes (prepared statement — sin SQL injection)
    const char *sql_solape =
        "SELECT franja_inicio, franja_fin FROM Reserva "
        "WHERE id_espacio=? AND fecha=? AND cancelada=0;";

    int solapa = 0;
    if (sqlite3_prepare_v2(db, sql_solape, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int (stmt, 1, id_espacio);
        sqlite3_bind_text(stmt, 2, fecha, -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *ri = (const char*)sqlite3_column_text(stmt, 0);
            const char *rf = (const char*)sqlite3_column_text(stmt, 1);
            if (hay_solapamiento(inicio, fin, ri, rf)) { solapa = 1; break; }
        }
        sqlite3_finalize(stmt);
    }
    if (solapa) {
        snprintf(respuesta, MAXDATASIZE, "ERROR|La reserva se solapa con otra existente en ese espacio");
        return;
    }

    const char *sql_ins =
        "INSERT INTO Reserva (id_espacio, dni_ciudadano, fecha, franja_inicio, "
        "franja_fin, num_personas, cancelada) VALUES (?, ?, ?, ?, ?, ?, 0);";
    if (sqlite3_prepare_v2(db, sql_ins, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int (stmt, 1, id_espacio);
        sqlite3_bind_text(stmt, 2, dni,     -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, fecha,   -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, inicio,  -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, fin,     -1, SQLITE_STATIC);
        sqlite3_bind_int (stmt, 6, num_personas);
        if (sqlite3_step(stmt) == SQLITE_DONE)
            snprintf(respuesta, MAXDATASIZE, "OK|Reserva creada correctamente");
        else
            snprintf(respuesta, MAXDATASIZE, "ERROR|No se pudo crear la reserva: %s", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
    }
}

static void handle_cancelar_reserva(const char *dni, const char *id_str, char *respuesta) {
    int id_reserva = atoi(id_str);
    sqlite3_stmt *stmt;

    // Verificar que la reserva existe, pertenece al cliente y no está ya cancelada
    const char *sql_check = "SELECT cancelada FROM Reserva WHERE id_reserva=? AND dni_ciudadano=?;";
    int cancelada = -1;
    if (sqlite3_prepare_v2(db, sql_check, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int (stmt, 1, id_reserva);
        sqlite3_bind_text(stmt, 2, dni, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW)
            cancelada = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    if (cancelada == -1) {
        snprintf(respuesta, MAXDATASIZE, "ERROR|Reserva no encontrada o no te pertenece");
        return;
    }
    if (cancelada == 1) {
        snprintf(respuesta, MAXDATASIZE, "ERROR|La reserva ya estaba cancelada");
        return;
    }

    const char *sql_upd = "UPDATE Reserva SET cancelada=1 WHERE id_reserva=?;";
    if (sqlite3_prepare_v2(db, sql_upd, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id_reserva);
        if (sqlite3_step(stmt) == SQLITE_DONE)
            snprintf(respuesta, MAXDATASIZE, "OK|Reserva %d cancelada correctamente", id_reserva);
        else
            snprintf(respuesta, MAXDATASIZE, "ERROR|No se pudo cancelar la reserva");
        sqlite3_finalize(stmt);
    }
}

// ─── NOTICIAS ─────────────────────────────────────────────────────────────

// categoria puede ser "Deportes", "Politica" o NULL/vacío para todas
static void handle_listar_noticias(const char *categoria, char *respuesta) {
    sqlite3_stmt *stmt;
    char buf[MAXDATASIZE];
    int pos = snprintf(buf, sizeof(buf), "OK");

    const char *sql_todas = "SELECT id_publicacion, categoria, titulo, enlace, fecha_publicacion, dni_admin "
                            "FROM Publicacion WHERE estado='ACTIVA';";
    const char *sql_cat   = "SELECT id_publicacion, categoria, titulo, enlace, fecha_publicacion, dni_admin "
                            "FROM Publicacion WHERE categoria=? AND estado='ACTIVA';";

    int por_categoria = categoria && categoria[0] != '\0';

    if (sqlite3_prepare_v2(db, por_categoria ? sql_cat : sql_todas, -1, &stmt, NULL) == SQLITE_OK) {
        if (por_categoria) sqlite3_bind_text(stmt, 1, categoria, -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW && pos < (int)sizeof(buf) - 400) {
            const char *enlace = (const char*)sqlite3_column_text(stmt, 3);
            const char *dni_admin = (const char*)sqlite3_column_text(stmt, 5);
            pos += snprintf(buf + pos, sizeof(buf) - pos, "\n%d;%s;%s;%s;%s;%s",
                sqlite3_column_int(stmt,  0),
                sqlite3_column_text(stmt, 1),
                sqlite3_column_text(stmt, 2),
                enlace ? enlace : "",
                sqlite3_column_text(stmt, 4),
                dni_admin ? dni_admin : "");
        }
        sqlite3_finalize(stmt);
    }
    snprintf(buf + pos, sizeof(buf) - pos, "\nEND");
    strncpy(respuesta, buf, MAXDATASIZE - 1);
}

// ─── LICENCIAS ────────────────────────────────────────────────────────────

static void handle_listar_tipos_licencia(char *respuesta) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id_tipo, nombre, descripcion, requisitos "
                      "FROM TipoLicencia WHERE activo=1;";
    char buf[MAXDATASIZE];
    int pos = snprintf(buf, sizeof(buf), "OK");

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW && pos < (int)sizeof(buf) - 400) {
            pos += snprintf(buf + pos, sizeof(buf) - pos, "\n%d;%s;%s;%s",
                sqlite3_column_int(stmt,  0),
                sqlite3_column_text(stmt, 1),
                sqlite3_column_text(stmt, 2),
                sqlite3_column_text(stmt, 3));
        }
        sqlite3_finalize(stmt);
    }
    snprintf(buf + pos, sizeof(buf) - pos, "\nEND");
    strncpy(respuesta, buf, MAXDATASIZE - 1);
}

static void handle_solicitar_licencia(const char *dni, const char *id_tipo_str, char *respuesta) {
    int id_tipo = atoi(id_tipo_str);
    sqlite3_stmt *stmt;

    // Verificar que el tipo existe y está activo
    int existe = 0;
    const char *sql_check = "SELECT COUNT(*) FROM TipoLicencia WHERE id_tipo=? AND activo=1;";
    if (sqlite3_prepare_v2(db, sql_check, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id_tipo);
        if (sqlite3_step(stmt) == SQLITE_ROW) existe = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    if (!existe) {
        snprintf(respuesta, MAXDATASIZE, "ERROR|Tipo de licencia no valido o inactivo");
        return;
    }

    // Verificar que no hay licencia duplicada activa/en revisión
    int dup = 0;
    const char *sql_dup =
        "SELECT COUNT(*) FROM Licencia "
        "WHERE dni_ciudadano=? AND id_tipo=? AND estado IN ('En revision','Aprobada');";
    if (sqlite3_prepare_v2(db, sql_dup, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, dni, -1, SQLITE_STATIC);
        sqlite3_bind_int (stmt, 2, id_tipo);
        if (sqlite3_step(stmt) == SQLITE_ROW) dup = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    if (dup) {
        snprintf(respuesta, MAXDATASIZE, "ERROR|Ya tienes una licencia activa o en revision de ese tipo");
        return;
    }

    char fecha_sol[16], fecha_exp[16];
    time_t ahora = time(NULL);
    strftime(fecha_sol, sizeof(fecha_sol), "%Y-%m-%d", localtime(&ahora));
    ahora += 157766400;  // +5 años en segundos
    strftime(fecha_exp, sizeof(fecha_exp), "%Y-%m-%d", localtime(&ahora));

    const char *sql_ins =
        "INSERT INTO Licencia (id_tipo, dni_ciudadano, estado, fecha_solicitud, fecha_expiracion) "
        "VALUES (?, ?, 'En revision', ?, ?);";
    if (sqlite3_prepare_v2(db, sql_ins, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int (stmt, 1, id_tipo);
        sqlite3_bind_text(stmt, 2, dni,       -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, fecha_sol, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, fecha_exp, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_DONE)
            snprintf(respuesta, MAXDATASIZE, "OK|Solicitud de licencia enviada correctamente");
        else
            snprintf(respuesta, MAXDATASIZE, "ERROR|No se pudo solicitar la licencia: %s", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
    }
}

static void handle_cambiar_password(const char *dni, const char *pass_actual,
                                     const char *nueva_pass, char *respuesta) {
    sqlite3_stmt *stmt;
    char hash_db[65] = "", fecha_db[32] = "";

    const char *sql_sel = "SELECT password, fecha_creacion FROM Cliente WHERE dni=? AND activo=1;";
    if (sqlite3_prepare_v2(db, sql_sel, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, dni, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *h = (const char*)sqlite3_column_text(stmt, 0);
            const char *f = (const char*)sqlite3_column_text(stmt, 1);
            if (h) strncpy(hash_db,  h, sizeof(hash_db)  - 1);
            if (f) strncpy(fecha_db, f, sizeof(fecha_db) - 1);
        }
        sqlite3_finalize(stmt);
    }

    if (!hash_db[0] || !fecha_db[0]) {
        snprintf(respuesta, MAXDATASIZE, "ERROR|No se encontro el usuario");
        return;
    }

    char hash_calc[65];
    auth_generar_hash(pass_actual, fecha_db, hash_calc);
    if (strcmp(hash_db, hash_calc) != 0) {
        snprintf(respuesta, MAXDATASIZE, "ERROR|Contrasena actual incorrecta");
        return;
    }

    // Mismo fecha_creacion como salt para mantener consistencia
    char nuevo_hash[65];
    auth_generar_hash(nueva_pass, fecha_db, nuevo_hash);

    const char *sql_upd = "UPDATE Cliente SET password=? WHERE dni=?;";
    if (sqlite3_prepare_v2(db, sql_upd, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, nuevo_hash, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, dni,        -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_DONE)
            snprintf(respuesta, MAXDATASIZE, "OK|Contrasena actualizada correctamente");
        else
            snprintf(respuesta, MAXDATASIZE, "ERROR|No se pudo actualizar la contrasena");
        sqlite3_finalize(stmt);
    }
}

static void handle_mis_licencias(const char *dni, char *respuesta) {
    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT l.id_licencia, t.nombre, l.estado, l.fecha_solicitud, l.fecha_expiracion "
        "FROM Licencia l JOIN TipoLicencia t ON l.id_tipo=t.id_tipo "
        "WHERE l.dni_ciudadano=?;";
    char buf[MAXDATASIZE];
    int pos = snprintf(buf, sizeof(buf), "OK");

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, dni, -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW && pos < (int)sizeof(buf) - 200) {
            pos += snprintf(buf + pos, sizeof(buf) - pos, "\n%d;%s;%s;%s;%s",
                sqlite3_column_int(stmt,  0),
                sqlite3_column_text(stmt, 1),
                sqlite3_column_text(stmt, 2),
                sqlite3_column_text(stmt, 3),
                sqlite3_column_text(stmt, 4));
        }
        sqlite3_finalize(stmt);
    }
    snprintf(buf + pos, sizeof(buf) - pos, "\nEND");
    strncpy(respuesta, buf, MAXDATASIZE - 1);
}

// ─── MANEJADOR DE CONEXIÓN ────────────────────────────────────────────────────

static void manejar_cliente(int fd) {
    char buffer[MAXDATASIZE];
    char respuesta[MAXDATASIZE];
    char dni_sesion[32]    = "";
    char nombre_sesion[64] = "";
    int  autenticado = 0;

    while (1) {
        memset(buffer,   0, MAXDATASIZE);
        memset(respuesta, 0, MAXDATASIZE);

        int recibido = recv(fd, buffer, MAXDATASIZE - 1, 0);
        if (recibido == 0) {
            if (autenticado) log_escribir("Fin de sesion (conexion cerrada)");
            break;
        }
        if (recibido == -1) { perror("recv"); break; }

        buffer[recibido] = '\0';
        buffer[strcspn(buffer, "\r\n")] = '\0';
        char copia[MAXDATASIZE];
        strncpy(copia, buffer, MAXDATASIZE - 1);
        char *cmd = strtok(copia, "|");

        if (!cmd || !cmd[0]) {
            snprintf(respuesta, MAXDATASIZE, "ERROR|Comando vacio");
            send(fd, respuesta, strlen(respuesta), 0);
            continue;
        }

        // ── Sin sesión requerida ──────────────────────────────────────────────
        if (strcmp(cmd, "LOGIN") == 0) {
            char *usuario  = strtok(NULL, "|");
            char *password = strtok(NULL, "|");
            if (usuario && password) {
                if (handle_login_no_existe(usuario)) {
                    strncpy(respuesta, "USER_NOT_FOUND", MAXDATASIZE - 1);
                    log_login_escribir(usuario, "Intento de login fallido: usuario no existe");
                } else {
                    autenticado = handle_login(usuario, password, dni_sesion, respuesta);
                    if (autenticado) {
                        strncpy(nombre_sesion, usuario, sizeof(nombre_sesion) - 1);
                        log_set_usuario(nombre_sesion);
                        log_escribir("Inicio de sesion");
                    } else {
                        log_login_escribir(usuario, "Intento de login fallido: contrasena incorrecta");
                    }
                }
            } else {
                snprintf(respuesta, MAXDATASIZE, "ERROR|LOGIN: faltan parametros");
            }
        }
        else if (strcmp(cmd, "REGISTER_CLIENTE") == 0) {
            char *dni      = strtok(NULL, "|");
            char *usuario  = strtok(NULL, "|");
            char *password = strtok(NULL, "|");
            char *fecha_nac = strtok(NULL, "|\r\n");
            if (dni && usuario && password) {
                handle_register(dni, usuario, password, fecha_nac, respuesta);
                if (strncmp(respuesta, "OK", 2) == 0)
                    log_login_escribir(usuario, "Nuevo cliente registrado");
            } else {
                snprintf(respuesta, MAXDATASIZE, "ERROR|REGISTER: faltan parametros");
            }
        }
        else if (strcmp(cmd, "exit") == 0) {
            if (autenticado) log_escribir("Fin de sesion");
            break;
        }
        // ── Requieren sesión iniciada ─────────────────────────────────────────
        else if (!autenticado) {
            snprintf(respuesta, MAXDATASIZE, "ERROR|Debes iniciar sesion primero");
        }
        // ── Espacios ──────────────────────────────────────────────────────────
        else if (strcmp(cmd, "LISTAR_ESPACIOS") == 0) {
            handle_listar_espacios(respuesta);
        }
        // ── Reservas ──────────────────────────────────────────────────────────
        else if (strcmp(cmd, "MIS_RESERVAS") == 0) {
            handle_mis_reservas(dni_sesion, respuesta);
        }
        else if (strcmp(cmd, "CREAR_RESERVA") == 0) {
            char *id_esp    = strtok(NULL, "|");
            char *fecha     = strtok(NULL, "|");
            char *inicio    = strtok(NULL, "|");
            char *fin       = strtok(NULL, "|");
            char *personas  = strtok(NULL, "|");
            if (id_esp && fecha && inicio && fin && personas) {
                handle_crear_reserva(dni_sesion, id_esp, fecha, inicio, fin, personas, respuesta);
                if (strncmp(respuesta, "OK", 2) == 0) {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "Ha creado una reserva: espacio %s, %s %s-%s", id_esp, fecha, inicio, fin);
                    log_escribir(msg);
                }
            } else {
                snprintf(respuesta, MAXDATASIZE, "ERROR|CREAR_RESERVA: faltan parametros");
            }
        }
        else if (strcmp(cmd, "CANCELAR_RESERVA") == 0) {
            char *id_res = strtok(NULL, "|");
            if (id_res) {
                handle_cancelar_reserva(dni_sesion, id_res, respuesta);
                if (strncmp(respuesta, "OK", 2) == 0) {
                    char msg[64];
                    snprintf(msg, sizeof(msg), "Ha cancelado la reserva ID %s", id_res);
                    log_escribir(msg);
                }
            } else {
                snprintf(respuesta, MAXDATASIZE, "ERROR|CANCELAR_RESERVA: falta ID");
            }
        }
        // ── Noticias ──────────────────────────────────────────────────────────
        else if (strcmp(cmd, "LISTAR_NOTICIAS") == 0) {
            char *categoria = strtok(NULL, "|");  // puede ser NULL (todas)
            // Limpiar \r\n que el cliente deja pegado al final de la categoria
            if (categoria) { char *nl = strpbrk(categoria, "\r\n"); if (nl) *nl = '\0'; }
            handle_listar_noticias(categoria, respuesta);
        }
        // ── Licencias ─────────────────────────────────────────────────────────
        else if (strcmp(cmd, "LISTAR_TIPOS_LICENCIA") == 0) {
            handle_listar_tipos_licencia(respuesta);
        }
        else if (strcmp(cmd, "SOLICITAR_LICENCIA") == 0) {
            char *id_tipo = strtok(NULL, "|");
            if (id_tipo) {
                handle_solicitar_licencia(dni_sesion, id_tipo, respuesta);
                if (strncmp(respuesta, "OK", 2) == 0) {
                    char msg[64];
                    snprintf(msg, sizeof(msg), "Ha solicitado una licencia de tipo ID %s", id_tipo);
                    log_escribir(msg);
                }
            } else {
                snprintf(respuesta, MAXDATASIZE, "ERROR|SOLICITAR_LICENCIA: falta ID de tipo");
            }
        }
        else if (strcmp(cmd, "MIS_LICENCIAS") == 0) {
            handle_mis_licencias(dni_sesion, respuesta);
        }
        else if (strcmp(cmd, "CAMBIAR_PASSWORD") == 0) {
            char *pass_actual = strtok(NULL, "|");
            char *nueva_pass  = strtok(NULL, "|");
            if (pass_actual && nueva_pass) {
                handle_cambiar_password(dni_sesion, pass_actual, nueva_pass, respuesta);
                if (strncmp(respuesta, "OK", 2) == 0)
                    log_escribir("Ha cambiado su contrasena");
            } else {
                snprintf(respuesta, MAXDATASIZE, "ERROR|CAMBIAR_PASSWORD: faltan parametros");
            }
        }
        else {
            snprintf(respuesta, MAXDATASIZE, "ERROR|Comando desconocido: %s", cmd);
        }

        // Añadir terminador \nEND si la respuesta no lo tiene ya (las listas lo incluyen)
        size_t rlen = strlen(respuesta);
        if (rlen + 4 < MAXDATASIZE && strstr(respuesta, "\nEND") == NULL)
            memcpy(respuesta + rlen, "\nEND", 5);

        if (send(fd, respuesta, strlen(respuesta), 0) == -1) {
            perror("send"); break;
        }
    }
    close(fd);
}
#endif // CLIENT_MODE


// ─── MAIN ─────────────────────────────────────────────────────────────────────

#ifdef SERVER_MAIN
int main(void) {
    int sockfd = -1;
    struct addrinfo hints, *servinfo, *p;
    int rv, yes = 1;
    if (!config_cargar("./server.conf")) {
        fprintf(stderr, "Error leyendo server.conf\n");
        return 1;
    }
    char port[16];
    snprintf(port, sizeof(port), "%d", server_get_puerto());
    if (!db_abrir(config.db_ruta)) {
        fprintf(stderr, "Error abriendo base de datos\n");
        return 1;
    }
    db_crear_tablas();

    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) { perror("socket"); continue; }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)  { perror("setsockopt"); close(sockfd); continue; }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)                         { perror("bind"); close(sockfd); continue; }
        break;
    }
    freeaddrinfo(servinfo);
    if (!p) { fprintf(stderr, "Fallo al hacer bind\n"); return 2; }
    if (listen(sockfd, BACKLOG) == -1) { perror("listen"); close(sockfd); return 3; }
    server_set_estado(1);

    // Evitar procesos zombie al terminar hijos
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    while (1) {
        struct sockaddr_storage addr;
        socklen_t addrlen = sizeof addr;

        int fd = accept(sockfd, (struct sockaddr*)&addr, &addrlen);
        if (fd == -1) { perror("accept"); continue; }

        pid_t pid = fork();
        if (pid == 0) {
            // Proceso hijo: cerrar y reabrir la BD para tener su propia conexión
            close(sockfd);
            db_cerrar();
            db_abrir("./ayuntamiento.db");
            manejar_cliente(fd);
            db_cerrar();
            exit(0);
        }
        // Proceso padre: cerrar el fd del cliente y seguir aceptando
        close(fd);
    }

    db_cerrar();
    server_set_estado(0);
    close(sockfd);
    return 0;
}
#endif
