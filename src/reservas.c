#include "../include/reservas.h"
#include "../include/db.h"
#include "../include/log.h"
#include "../include/config.h"
#include "../include/funciones.h"
#include "../include/espacios.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ===== CALLBACKS PARA CONSULTAS ===== */

static int callback_listar_reservas(void *data, int cols, char **valores, char **nombres) {
    printf("  [ID: %s] Espacio: %s | DNI: %s | Fecha: %s | Entrada: %s - Salida: %s\n",
        valores[0],              // id_reserva
        valores[1],              // id_espacio
        valores[2],              // dni_ciudadano
        valores[3],              // fecha
        valores[4],              // franja_inicio
        valores[5]               // franja_fin
    );
    printf("    Personas: %s | Estado: %s\n",
        valores[6],              // num_personas
        atoi(valores[7]) ? "CANCELADA" : "ACTIVA"  // cancelada
    );
    printf("  ---\n");
    return 0;
}

static int callback_contar_reservas(void *data, int cols, char **valores, char **nombres) {
    int *total = (int *)data;
    if (valores[0]) {
        *total = atoi(valores[0]);
    }
    return 0;
}

static int callback_obtener_id(void *data, int cols, char **valores, char **nombres) {
    int *id = (int *)data;
    if (valores[0]) {
        *id = atoi(valores[0]);
    }
    return 0;
}

int callback_capacidad(void *data, int argc, char **argv, char **col) {
    int *cap = (int *)data;

    if (argc > 0 && argv[0]) {
        *cap = atoi(argv[0]);
    }

    return 0;
}

//Funciones auxiliaares para el control de solapamiento de reservas


//  (HH:MM) --> minutos totales ( Mas facil comparar numeros que string para el solapamiento)
int hora_a_minutos(const char *hora) {
    int horas, minutos;
    sscanf(hora, "%d:%d", &horas, &minutos);
    return horas * 60 + minutos;
}

//  -1 si hora1 es menor , 0 si  son iguales, 1 si hora1 es mayor
int comparar_horas(const char *hora1, const char *hora2) {
    int min1 = hora_a_minutos(hora1);
    int min2 = hora_a_minutos(hora2);
    if (min1 < min2) return -1;
    if (min1 > min2) return 1;
    return 0;
}

//Valida si cumple con el horario del confg
int validar_horario(const char *hora) {
    int min_hora = hora_a_minutos(hora);
    int min_apertura = hora_a_minutos(get_apertura());
    int min_cierre = hora_a_minutos(get_cierre());
    
    return (min_hora >= min_apertura && min_hora <= min_cierre);
}

// retorna 1 si se solapan, 0 si no
int hay_solapamiento(const char *inicio1, const char *fin1, const char *inicio2, const char *fin2) {
    int min_inicio1 = hora_a_minutos(inicio1);
    int min_fin1 = hora_a_minutos(fin1);
    int min_inicio2 = hora_a_minutos(inicio2);
    int min_fin2 = hora_a_minutos(fin2);
    
    return (min_inicio1 < min_fin2 && min_inicio2 < min_fin1);
}

//Verificar el formato y rango de la hora
int verificarHora(char* hora) {
    int h, m;

    if (sscanf(hora, "%d:%d", &h, &m) != 2) {
        printf("[ERROR] Formato de hora invalido. Tiene que ser HH:MM.\n");
        return 1;
    }

    if (h < 0 || h > 23) {
        printf("[ERROR] La hora tiene que estar entre 00 y 23.\n");
        return 1;
    }

    if (m < 0 || m > 59) {
        printf("[ERROR] Los minutos tienen que estar entre 00 y 59.\n");
        return 1;
    }

    return 0; // Hora valida
}



/* CREATE - Crear nueva reserva */ //Esta se puede usar como base para cuado hagamos la de ciudadano agregando unos parametros determinados(dni ya metido etc)
void reservas_crear_Aciudadano() {
    int id_espacio;
    char dni_ciudadano[16];
    char fecha[16];
    char franja_inicio[6];
    char franja_fin[6];
    int num_personas;
    int capacidad_max;

    printf("\n--- CREAR NUEVA RESERVA A UN CIUDADANO ---\n");
    do {
        printf("DNI del ciudadano: ");
        scanf(" %15[^\n]", dni_ciudadano);
        limpiarBuffer();
    } while (!dni_es_valido(dni_ciudadano));

    printf("ID del espacio: ");
    scanf("%d", &id_espacio);
    getchar();  // Limpiar buffer
    
  // Verificar que el espacio existe y está activo
    char sql_check[256];
    snprintf(sql_check, sizeof(sql_check),
        "SELECT COUNT(*)  FROM Espacio WHERE id_espacio=%d AND activo=1;", id_espacio);

    int existe = 0;
    sqlite3_exec(db, sql_check, callback_contar_reservas, &existe, NULL);

    if (existe == 0) {
        printf("[ERROR] El espacio con ID %d no existe o está dado de baja.\n", id_espacio);
        return;
    }

    do {
        printf("Fecha de reserva (YYYY-MM-DD): ");
        scanf(" %15[^\n]", fecha);
        limpiarBuffer();
    } while (!fecha_es_valida(fecha) || !fecha_es_hoy_o_posterior(fecha));

    printf("\tHorario de apertura: de %s a %s\n",get_apertura(),get_cierre());

    do{
    printf("Hora de entrada (HH:MM): ");
    scanf(" %5[^\n]", franja_inicio);
    getchar();
    }while(verificarHora(franja_inicio) == 1);

    //Verificacion de la hora de entrada (server.conf)
    if (!validar_horario(franja_inicio)) {
        printf("[ERROR] Hora de entrada (%s) fuera del horario. El sistema funciona de 09:00 a 21:00.\n",
            franja_inicio);
        return;
    }

    do{
    printf("Hora de salida (HH:MM): ");
    scanf(" %5[^\n]", franja_fin);
    getchar();
    }while(verificarHora(franja_fin) == 1);


    //salida
    if (!validar_horario(franja_fin)) {
        printf("[ERROR] Hora de salida (%s) fuera del horario. El sistema funciona de 09:00 a 21:00.\n",
            franja_fin);
        return;
    }

    //Verificamos que tenga sentido las horas que ha metido
    if (comparar_horas(franja_inicio, franja_fin) >= 0) {
        printf("[ERROR] La hora de entrada debe ser anterior a la hora de salida.\n");
        return;
    }

    //Verificacion TOCHA que mira en la base de datos
    typedef struct { //Esto a futuro igual cambiar a objetos 
        char inicio[6];
        char fin[6];
    } Franja;
    
    Franja franjas[24]; // Contando qeu como mucho se reservan cada 30 mins -- Si no argegar mas
    int contador = 0;
 
    char sql_query[512];
    snprintf(sql_query, sizeof(sql_query),
        "SELECT franja_inicio, franja_fin FROM Reserva "
        "WHERE id_espacio=%d AND fecha='%s' AND cancelada=0;",
        id_espacio, fecha);
 
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql_query, -1, &stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            if (contador < 24) {  //Max de franjas 
                strcpy(franjas[contador].inicio, (const char *)sqlite3_column_text(stmt, 0));
                strcpy(franjas[contador].fin, (const char *)sqlite3_column_text(stmt, 1));
                contador++;
            }
        }
        sqlite3_finalize(stmt);
    }

    for (int i = 0; i < contador; i++) {
        if (hay_solapamiento(franja_inicio, franja_fin, franjas[i].inicio, franjas[i].fin)) {
            printf("[ERROR] La reserva se solapa con otra existente (%s-%s).\n",
                franjas[i].inicio, franjas[i].fin);
            return;
        }
    }


    char sql_capa[256];
    snprintf(sql_capa, sizeof(sql_capa), "SELECT capacidad FROM Espacio WHERE id_espacio=%d AND activo=1;", id_espacio);

    sqlite3_exec(db, sql_capa, callback_capacidad, &capacidad_max, NULL);

    printf("El espacio tiene una capacidad maxima de %d personas\n", capacidad_max);

    printf("Número de personas: ");
    scanf("%d", &num_personas);
    getchar();

    if (num_personas > capacidad_max) {
    printf("[ERROR] El espacio solo admite %d personas.\n", capacidad_max);
    return;
    }

    
  

    // Insertar la reserva
    char sql[512];
    snprintf(sql, sizeof(sql),
        "INSERT INTO Reserva (id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin, num_personas, cancelada) "
        "VALUES (%d, '%s', '%s', '%s', '%s', %d, 0);",
        id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin, num_personas);

    if (db_ejecutar(sql)) {
        printf("[OK] Reserva creada correctamente.\n");
        printf("     Espacio: %d | DNI: %s | Fecha: %s | %s-%s\n",
            id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin);

        char msg[256];
        snprintf(msg, sizeof(msg),
            "Ha creado una nueva reserva para el espacio %d para el ciudadano con Dni: %s)",
            id_espacio, dni_ciudadano);
        log_escribir(msg);
    } else {
        printf("[ERROR] No se pudo crear la reserva.\n");
    }
}

/* READ - Listar todas las reservas */
void reservas_listar_todas() {
    printf("\n--- LISTADO DE TODAS LAS RESERVAS ---\n");

    char *err = NULL;
    int resultado = sqlite3_exec(db,
        "SELECT id_reserva, id_espacio, dni_ciudadano, fecha, franja_inicio, "
        "franja_fin, num_personas, cancelada FROM Reserva ORDER BY fecha DESC;",
        callback_listar_reservas, NULL, &err);

    if (resultado != SQLITE_OK) {
        printf("[ERROR] %s\n", err);
        sqlite3_free(err);
    }

    // Mostrar estadísticas
    int total_reservas = 0;
    sqlite3_exec(db, "SELECT COUNT(*) FROM Reserva;", callback_contar_reservas, &total_reservas, NULL);

    int activas = 0;
    sqlite3_exec(db, "SELECT COUNT(*) FROM Reserva WHERE cancelada=0;", callback_contar_reservas, &activas, NULL);

    printf("  Total de reservas: %d | Activas: %d | Canceladas: %d\n",
        total_reservas, activas, total_reservas - activas);
    log_escribir("Ha consultado el listado completo de reservas");

}

/* READ - Listar reservas de un espacio específico */
void reservas_listar_por_espacio() {
    int id_espacio;
    
    espacios_listar();
    printf("\n--- VER RESERVAS DE UN ESPACIO ---\n");
    printf("ID del espacio: ");
    scanf("%d", &id_espacio);
    getchar();

    // Verificar que el espacio existe
    char sql_check[256];
    snprintf(sql_check, sizeof(sql_check),
        "SELECT nombre FROM Espacio WHERE id_espacio=%d;", id_espacio);

    char nombre_espacio[128] = "Desconocido";

    // Callback auxiliar para obtener nombre
    struct {
        char *nombre;
    } contexto = { nombre_espacio };

    // Simplificado: ejecutar directamente
    printf("\n--- RESERVAS DEL ESPACIO %d ---\n", id_espacio);

    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT id_reserva, id_espacio, dni_ciudadano, fecha, franja_inicio, "
        "franja_fin, num_personas, cancelada FROM Reserva "
        "WHERE id_espacio=%d ORDER BY fecha DESC;",
        id_espacio);

    char *err = NULL;
    int resultado = sqlite3_exec(db, sql, callback_listar_reservas, NULL, &err);

    if (resultado != SQLITE_OK) {
        printf("[ERROR] %s\n", err);
        sqlite3_free(err);
    }

    // Estadísticas por espacio
    int total = 0;
    snprintf(sql, sizeof(sql),
        "SELECT COUNT(*) FROM Reserva WHERE id_espacio=%d;", id_espacio);
    sqlite3_exec(db, sql, callback_contar_reservas, &total, NULL);

    int activas = 0;
    snprintf(sql, sizeof(sql),
        "SELECT COUNT(*) FROM Reserva WHERE id_espacio=%d AND cancelada=0;", id_espacio);
    sqlite3_exec(db, sql, callback_contar_reservas, &activas, NULL);

    if (total == 0) {
        printf("\n[INFO] Este espacio no tiene reservas registradas.\n");
    } else {
        printf("\nTotal de reservas: %d | Activas: %d | Canceladas: %d\n",
            total, activas, total - activas);
    }
}

/* UPDATE - Cancelar una reserva */
void reservas_cancelar() {
    int id_reserva;

    printf("\n--- CANCELAR RESERVA ---\n");

    // Primero listar todas para que el usuario vea los IDs
    printf("Reservas existentes:\n");
    reservas_listar_todas();

    printf("\nID de la reserva a cancelar: ");
    scanf("%d", &id_reserva);
    getchar();

    // Verificar que la reserva existe y no está ya cancelada
    char sql_check[256];
    snprintf(sql_check, sizeof(sql_check),
        "SELECT cancelada FROM Reserva WHERE id_reserva=%d;", id_reserva);

    int estado_cancelada = -1;
    sqlite3_exec(db, sql_check, callback_obtener_id, &estado_cancelada, NULL);

    if (estado_cancelada == -1) {
        printf("[ERROR] No existe ninguna reserva con ID %d.\n", id_reserva);
        return;
    }

    if (estado_cancelada == 1) {
        printf("[ADVERTENCIA] Esta reserva ya está cancelada.\n");
        return;
    }

    // Cancelar la reserva (marcar como cancelada)
    char sql[256];
    snprintf(sql, sizeof(sql),
        "UPDATE Reserva SET cancelada=1 WHERE id_reserva=%d;", id_reserva);

    if (db_ejecutar(sql)) {
        printf("[OK] Reserva %d cancelada correctamente.\n", id_reserva);

        char msg[256];
        snprintf(msg, sizeof(msg), "Ha cancelado la reserva con ID %d", id_reserva);
        log_escribir(msg);
    } else {
        printf("[ERROR] No se pudo cancelar la reserva.\n");
    }
}

/* UPDATE - Editar detalles de una reserva */
void reservas_editar() {
    int id_reserva;
    int opcion_editar;

    printf("\n--- EDITAR RESERVA ---\n");

    // Listar todas las reservas
    printf("Reservas existentes:\n");
    reservas_listar_todas();

    printf("\nID de la reserva a editar: ");
    scanf("%d", &id_reserva);
    getchar();

    // Verificar que la reserva existe
    char sql_check[256];
    snprintf(sql_check, sizeof(sql_check),
        "SELECT id_reserva FROM Reserva WHERE id_reserva=%d;", id_reserva);

    int existe = 0;
    sqlite3_exec(db, sql_check, callback_contar_reservas, &existe, NULL);

    if (existe == 0) {
        printf("[ERROR] No existe ninguna reserva con ID %d.\n", id_reserva);
        return;
    }

    // Menú de opciones de edición
    printf("\n--- SELECCIONA QUÉ EDITAR ---\n");
    printf("1. Cambiar DNI del ciudadano\n");
    printf("2. Cambiar fecha\n");
    printf("3. Cambiar hora de entrada\n");
    printf("4. Cambiar hora de salida\n");
    printf("5. Cambiar número de personas\n");
    printf("0. Cancelar\n");
    printf("Opción: ");
    scanf("%d", &opcion_editar);
    getchar();

    char sql_update[512];
    char nuevo_valor[128];

    switch (opcion_editar) {
        case 1:
            do {
                printf("Nuevo DNI: ");
                scanf(" %127[^\n]", nuevo_valor);
                limpiarBuffer();
            } while (!dni_es_valido(nuevo_valor));
            snprintf(sql_update, sizeof(sql_update),
                "UPDATE Reserva SET dni_ciudadano='%s' WHERE id_reserva=%d;",
                nuevo_valor, id_reserva);
            break;
        case 2:
            do {
                printf("Nueva fecha (YYYY-MM-DD): ");
                scanf(" %127[^\n]", nuevo_valor);
                limpiarBuffer();
            } while (!fecha_es_valida(nuevo_valor) || !fecha_es_hoy_o_posterior(nuevo_valor));
            snprintf(sql_update, sizeof(sql_update),
                "UPDATE Reserva SET fecha='%s' WHERE id_reserva=%d;",
                nuevo_valor, id_reserva);
            break;

        case 3:
            do{
            printf("Nueva hora de entrada (HH:MM): ");
            scanf(" %10[^\n]", nuevo_valor);
            getchar();
            }while(verificarHora(nuevo_valor) == 1);
                if (!validar_horario(nuevo_valor)) {
                printf("[ERROR] Hora fuera del horario permitido.\n");
                return;
            }
            snprintf(sql_update, sizeof(sql_update),
                "UPDATE Reserva SET franja_inicio='%s' WHERE id_reserva=%d;",
                nuevo_valor, id_reserva);
            break;

        case 4:
            do{
            printf("Nueva hora de salida (HH:MM): ");
            scanf(" %127[^\n]", nuevo_valor);
            getchar();
            }while(verificarHora(nuevo_valor) == 1);
                if (!validar_horario(nuevo_valor)) {
                    printf("[ERROR] Hora fuera del horario permitido.\n");
                    return;
                }
            snprintf(sql_update, sizeof(sql_update),
                "UPDATE Reserva SET franja_fin='%s' WHERE id_reserva=%d;",
                nuevo_valor, id_reserva);
            break;
        case 5: {
            int nuevas_personas;
            printf("Nuevo número de personas: ");
            if (scanf("%d", &nuevas_personas) != 1 || nuevas_personas < 1) {
                printf("[ERROR] Numero de personas invalido.\n");
                limpiarBuffer();
                return;
            }
            getchar();
            
            snprintf(sql_update, sizeof(sql_update),
                "UPDATE Reserva SET num_personas=%d WHERE id_reserva=%d;",
                nuevas_personas, id_reserva);
            break;
            }
        case 0:
            printf("Edición cancelada.\n");
            return;
        default:
            printf("[ERROR] Opción no válida.\n");
            return;
    }

    if (db_ejecutar(sql_update)) {
        printf("[OK] Reserva %d actualizada correctamente.\n", id_reserva);
        char msg[256];
        snprintf(msg, sizeof(msg), "Ha editado los datos de la reserva con ID %d", id_reserva);
        log_escribir(msg);
    } else {
        printf("[ERROR] No se pudo actualizar la reserva.\n");
    }
}
