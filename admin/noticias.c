#include "../include/noticias.h"
#include "../include/funciones.h"
#include "../include/db.h"
#include "../include/log.h"
#include "../include/auth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void submenuNoticias()
{
    int opcion;
    do
    {
        printf("\n--- GESTION DE NOTICIAS ---\n");
        printf("1. Publicar Noticia\n");
        printf("2. Gestionar Noticia (editar/eliminar)\n");
        printf("3. Consultar Noticias\n");
        printf("0. Volver al menu principal\n");
        printf("Seleccion: ");

        if (scanf("%d", &opcion) != 1)
        {
            limpiarBuffer();
            opcion = 0;
        }

        switch (opcion)
        {
        case 1:
            noticia_publicar();
            break;
        case 2:
            noticia_gestionar();
            break;
        case 3:
            verNoticias();
            break;
        case 0:
            printf("\nVolviendo al menu principal...\n");
            break;
        default:
            printf("\n[!] Opcion invalida. Intenta de nuevo.\n");
        }
    } while (opcion != 0);
}







void noticia_publicar()
{
    char categoria[64];
    char titulo[256];
    char enlace[256];
    char fecha_publicacion[32];

    printf("\n--- PUBLICAR NOTICIA ---\n");

    int opcion_cat;
    do
    {
        printf("Categoria:\n");
        printf("  1. Deportes\n");
        printf("  2. Politica\n");
        printf("Seleccion: ");
        if (scanf("%d", &opcion_cat) != 1) { limpiarBuffer(); opcion_cat = 0; }
        else limpiarBuffer();
    } while (opcion_cat != 1 && opcion_cat != 2);

    if (opcion_cat == 1) strcpy(categoria, "Deportes");
    else                 strcpy(categoria, "Politica");

    do
    {
        printf("Titulo: ");
        scanf(" %255[^\n]", titulo);
        if (strlen(titulo) == 0)
        {
            printf("[ERROR] El titulo no puede estar vacio.\n");
        }
    } while (strlen(titulo) == 0);
    limpiarBuffer();

    printf("Enlace (opcional, pulsa Enter para omitir): ");
    fflush(stdout);
    if (fgets(enlace, sizeof(enlace), stdin) != NULL)
        enlace[strcspn(enlace, "\n")] = '\0';

    time_t ahora = time(NULL);
    struct tm *fecha = localtime(&ahora);
    strftime(fecha_publicacion, sizeof(fecha_publicacion), "%Y-%m-%d", fecha);

    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO Publicacion (categoria, titulo, enlace, dni_admin, fecha_publicacion, estado) "
                      "VALUES (?, ?, ?, ?, ?, 'ACTIVA');";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK)
    {
        // Vinculamos cada variable al marcador '?' correspondiente
        sqlite3_bind_text(stmt, 1, categoria, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, titulo, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, enlace, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, dni_admin_sesion, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, fecha_publicacion, -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_DONE)
        {
            printf("[OK] Noticia '%s' publicada correctamente.\n", titulo);

            char msg[512];
            snprintf(msg, sizeof(msg),
                     "Ha agregado a la BD una nueva publicacion titulada '%s'",
                     titulo);
            log_escribir(msg);
        }
        else
        {
            printf("[ERROR] No se pudo publicar la noticia: %s\n", sqlite3_errmsg(db));
        }
        
        sqlite3_finalize(stmt);
    }
    else
    {
        printf("[ERROR] Error al preparar la consulta: %s\n", sqlite3_errmsg(db));
    }
}

void noticia_gestionar()
{
    int id;

    printf("\n--- GESTIONAR NOTICIA ---\n");

    // Listar todas las activas
    char *err = NULL;
    int total = 0;
    noticia_listar();



    printf("\nID de la noticia: ");
    if (scanf("%d", &id) != 1)
    {
        printf("[ERROR] ID no valido.\n");
        limpiarBuffer();
        return;
    }
    limpiarBuffer();

    int accion;
    printf("\n1. Editar\n");
    printf("2. Eliminar\n");
    printf("0. Cancelar\n");
    printf("Seleccion: ");

    if (scanf("%d", &accion) != 1)
    {
        limpiarBuffer();
        return;
    }
    limpiarBuffer();

    // --- EDITAR ---
    if (accion == 1)
    {
        int campo;
        int editando = 1;

        while (editando)
        {
            printf("\n--- CAMPO A EDITAR (noticia ID %d) ---\n", id);
            printf("1. Categoria\n");
            printf("2. Titulo\n");
            printf("3. Enlace\n");
            printf("0. Terminar edicion\n");
            printf("Seleccion: ");

            if (scanf("%d", &campo) != 1)
            {
                limpiarBuffer();
                continue;
            }
            limpiarBuffer();

            if (campo == 0)
                break;

            char nuevo_valor[256];
            const char *nombre_campo_sql = NULL;
            const char *nombre_campo_log = NULL;

            switch (campo)
            {
            case 1: {
                int opcion_cat;
                do {
                    printf("Nueva categoria:\n");
                    printf("  1. Deportes\n");
                    printf("  2. Politica\n");
                    printf("Seleccion: ");
                    if (scanf("%d", &opcion_cat) != 1) { limpiarBuffer(); opcion_cat = 0; }
                    else limpiarBuffer();
                } while (opcion_cat != 1 && opcion_cat != 2);
                strcpy(nuevo_valor, opcion_cat == 1 ? "Deportes" : "Politica");
                nombre_campo_sql = "categoria";
                nombre_campo_log = "categoria";
                break;
            }
            case 2:
                printf("Nuevo titulo: ");
                nombre_campo_sql = "titulo";
                nombre_campo_log = "titulo";
                scanf(" %255[^\n]", nuevo_valor);
                limpiarBuffer();
                break;
            case 3:
                printf("Nuevo enlace: ");
                nombre_campo_sql = "enlace";
                nombre_campo_log = "enlace";
                scanf(" %255[^\n]", nuevo_valor);
                limpiarBuffer();
                break;
            default:
                printf("[!] Opcion invalida.\n");
                continue;
            }

            sqlite3_stmt *stmt;
            char sql_query[512];
            // El nombre de la columna se concatena porque es estático (viene de nuestro switch), 
            // pero el valor del usuario se protege con '?'
            snprintf(sql_query, sizeof(sql_query), "UPDATE Publicacion SET %s=? WHERE id_publicacion=? AND estado='ACTIVA';", nombre_campo_sql);

            if (sqlite3_prepare_v2(db, sql_query, -1, &stmt, NULL) == SQLITE_OK)
            {
                sqlite3_bind_text(stmt, 1, nuevo_valor, -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 2, id);

                if (sqlite3_step(stmt) == SQLITE_DONE)
                {
                    if (sqlite3_changes(db) > 0)
                    {
                        printf("[OK] Campo '%s' actualizado correctamente.\n", nombre_campo_log);
                        char msg[300];
                        snprintf(msg, sizeof(msg), "Ha editado el campo '%s' de la publicacion con ID %d", nombre_campo_log, id);
                        log_escribir(msg);
                    }
                    else
                    {
                        printf("[ERROR] No existe ninguna noticia activa con ID %d.\n", id);
                        editando = 0;
                    }
                }
                else
                {
                    printf("[ERROR] No se pudo actualizar el campo.\n");
                }
                sqlite3_finalize(stmt);
            }
        }
        printf("[INFO] Edicion finalizada para la noticia ID %d.\n", id);
    }

    // --- ELIMINAR ---
    if (accion == 2)
    {
        sqlite3_stmt *stmt;
        const char *sql_del = "DELETE FROM Publicacion WHERE id_publicacion=?;";

        if (sqlite3_prepare_v2(db, sql_del, -1, &stmt, NULL) == SQLITE_OK)
        {
            sqlite3_bind_int(stmt, 1, id);

            if (sqlite3_step(stmt) == SQLITE_DONE)
            {
                if (sqlite3_changes(db) > 0)
                {
                    printf("[OK] Noticia con ID %d eliminada correctamente.\n", id);
                    char msg[200];
                    snprintf(msg, sizeof(msg), "Ha eliminado la publicacion con ID %d", id);
                    log_escribir(msg);
                }
                else
                {
                    printf("[ERROR] No existe ninguna noticia con ID %d.\n", id);
                }
            }
            else
            {
                printf("[ERROR] No se pudo eliminar la noticia.\n");
            }
            sqlite3_finalize(stmt);
        }
        return;
    }
}
