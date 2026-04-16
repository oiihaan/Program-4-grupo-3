#include "../include/noticias.h"
#include "../include/funciones.h"
#include "../include/db.h"
#include "../include/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>

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

static int callback_mostrar_deportes(void *data, int cols, char **valores, char **nombres)
{
    int *contador = (int *)data;
    const char *id = (valores[0] && valores[0][0]) ? valores[0] : "-";
    const char *tipo = (valores[1] && valores[1][0]) ? valores[1] : "-";
    const char *titulo = (valores[2] && valores[2][0]) ? valores[2] : "-";
    const char *enlace = (valores[3] && valores[3][0]) ? valores[3] : "-";
    const char *fecha = (valores[4] && valores[4][0]) ? valores[4] : "-";
    const char *estado = (valores[5] && valores[5][0]) ? valores[5] : "-";

    (*contador)++;
    printf("  [%s] %-12s | %-30s | %-10s | %-8s\n",
           id, tipo, titulo, fecha, estado);
    printf("      Enlace: %s\n", enlace);
    return 0;
}

static int callback_listar_noticias(void *data, int cols, char **valores, char **nombres)
{
    int *contador = (int *)data;
    const char *id = (valores[0] && valores[0][0]) ? valores[0] : "-";
    const char *categoria = (valores[1] && valores[1][0]) ? valores[1] : "-";
    const char *tipo = (valores[2] && valores[2][0]) ? valores[2] : "-";
    const char *titulo = (valores[3] && valores[3][0]) ? valores[3] : "-";
    const char *fecha = (valores[4] && valores[4][0]) ? valores[4] : "-";
    const char *estado = (valores[5] && valores[5][0]) ? valores[5] : "-";

    (*contador)++;
    printf("  [%s] %-12s | %-12s | %-30s | %-10s | %-10s\n",
           id, categoria, tipo, titulo, fecha, estado);
    return 0;
}

void verNoticias()
{
    int opcion;
    do
    {
        printf("\n--- CONSULTA DE NOTICIAS ---\n");
        printf("1. Deportes\n");
        printf("2. El tiempo\n");
        printf("3. Listar todas las noticias\n");
        printf("0. Volver al menu gestion de noticias\n");
        printf("Seleccion: ");

        if (scanf("%d", &opcion) != 1)
        {
            limpiarBuffer();
            opcion = 0; // Forzamos un valor invalido
        }

        switch (opcion)
        {
        case 1:
            mostrarDeportes();
            break;
        case 2:
            mostrarTiempo();
            break;
        case 3:
            noticia_listar();
            break;
        case 0:
            printf("\nVolviendo al menu de gestion de noticias...\n");
            break;
        default:
            printf("\n[!] Opcion invalida. Intenta de nuevo.\n");
        }
    } while (opcion != 0);
}


// AQUI EMPIEZA LA PARTE DEL TIEMPO

// API de Open-Meteo
#define URL_TIEMPO "https://api.open-meteo.com/v1/forecast?latitude=43.3128&longitude=-1.975&daily=weather_code,temperature_2m_max,temperature_2m_min,rain_sum&timezone=Europe%2FBerlin"

// Buffer dinamico para acumular la respuesta HTTP
typedef struct
{
    char *datos;
    size_t tam; // Pongo long por si devuelve una cantidad enorme de info aunque es poco probable
} Respuesta;    // Todo lo que devuelve la API en Json

// Funcion para reservar el espacio necesario para la respuesta que nos manda la API
static size_t callback_curl(void *contenido, size_t tam, size_t nmemb, Respuesta *resp)
{
    size_t total = tam * nmemb; // Tamaño total (bytes)
    // Añade mas espacio al ya reservado
    resp->datos = realloc(resp->datos, resp->tam + total + 1);
    // Si se acaba la RAM, medida de seguridad de Claude
    if (!resp->datos)
        return 0;
    // Recibe los datos nuevos y los añade justo donde acababa antes
    memcpy(resp->datos + resp->tam, contenido, total);
    // Guarda el nuevo tamaño
    resp->tam += total;
    // Caracter nulo para que el ordenador sepa donde acaba el array
    resp->datos[resp->tam] = '\0';
    return total;
}

// Separa los datos del Json, como si leyera un CSV
static char *extraer_array(const char *json, const char *campo)
{

    char buscar[64];
    snprintf(buscar, sizeof(buscar), "\"%s\":[", campo); // Para buscar a que campo pertenece

    char *ini = strstr(json, buscar);
    if (!ini)
    {
        return NULL;
    }

    ini = strchr(ini, '[') + 1;   // Busca donde abre el corchete y empieza nueva info
    char *fin = strchr(ini, ']'); // Busca donde haya un corchete, asi corta el Json
    if (!fin)
    {
        return NULL;
    }

    // Calcular cuantos caracteres hay en medio
    size_t len = fin - ini;

    // Reserva espacio para el resultado
    char *resultado = malloc(len + 1); //+1 para caracter nulo

    // Solamente copia ini a resultado (StringCopy)
    // La n es porque se indica la longitud
    strncpy(resultado, ini, len);

    // Cierra el String
    resultado[len] = '\0';

    return resultado;
    // Hacer FREE luego
}

// Traduce el numero de codigoClima que nos da la web y lo traduce en palabras
// Esta funcion de es Claude entera
static const char *descripcion_clima(int codigo)
{
    if (codigo == 0)
        return "Despejado";
    if (codigo <= 3)
        return "Parcialmente nublado";
    if (codigo <= 48)
        return "Niebla";
    if (codigo <= 57)
        return "Llovizna";
    if (codigo <= 67)
        return "Lluvia";
    if (codigo <= 77)
        return "Nieve";
    if (codigo <= 82)
        return "Chubascos";
    if (codigo <= 99)
        return "Tormenta";
    return "Desconocido";
}

// Mostrar el Tiempo
void mostrarTiempo()
{
    printf("\n--- TIEMPO EN DONOSTI (proximos 7 dias) ---\n");

    // Inicializar curl
    CURL *curl = curl_easy_init();

    if (!curl)
    { // Prevencion de errores
        printf("[ERROR] No se pudo inicializar curl.\n");
        return;
    }

    // Giuardo el espacio para guardar la respuesta
    Respuesta resp;
    resp.datos = malloc(1); // Empieza con espacio minimo y luego añade mas segun la funcion de antes
    resp.tam = 0;
    resp.datos[0] = '\0';

    // Configurar curl
    curl_easy_setopt(curl, CURLOPT_URL, URL_TIEMPO);              // URL de la API
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback_curl); // Escribe los datos
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);             // Indica donde escribirlos
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);                 // Tiempo de espera de 10 segundos

    // Hacer la peticion (Descarga los datos)
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl); // Cerramos el motor

    if (res != CURLE_OK)
    { // Prevencion de errores
        printf("[ERROR] No se pudo obtener el tiempo: %s\n", curl_easy_strerror(res));
        free(resp.datos);
        return;
    }

    // Comprobar que la respuesta es JSON y no un error HTML
    if (strstr(resp.datos, "<html>") != NULL)
    {
        printf("[ERROR] El servidor de la API no esta disponible. Intentalo mas tarde.\n");
        free(resp.datos);
        return;
    }

    // Extraer cada array del JSON con la funcion de antes
    char *arr_fechas = extraer_array(resp.datos, "time");
    char *arr_max = extraer_array(resp.datos, "temperature_2m_max");
    char *arr_min = extraer_array(resp.datos, "temperature_2m_min");
    char *arr_lluvia = extraer_array(resp.datos, "rain_sum");
    char *arr_codigo = extraer_array(resp.datos, "weather_code");

    // Si hay algun dato que no aparece se corta
    if (!arr_fechas || !arr_max || !arr_min || !arr_lluvia || !arr_codigo)
    {
        printf("[ERROR] No se pudo parsear la respuesta.\n");
    }

    else
    { // Si todo esta correcto...
        // Rellenar array de structs Dia
        Dia dias[7];

        // Estos punteros son para recorrer la lista sin perder el inicio (Claude)
        char *pfec = arr_fechas;
        char *pmax = arr_max;
        char *pmin = arr_min;
        char *pllu = arr_lluvia;
        char *pcod = arr_codigo;

        for (int i = 0; i < 7; i++)
        {
            // La fecha esta entre comillas
            char *ini = strchr(pfec, '"'); // Primera comilla (Como corchetes antes)

            if (!ini)
            {
                break;
            }

            ini++;
            char *fin = strchr(ini, '"'); // Segunda comilla (cierre)

            if (!fin)
            {
                break;
            }

            strncpy(dias[i].fecha, ini, fin - ini); // Copia el trozo de la fecha
            dias[i].fecha[fin - ini] = '\0';        // Cierra el String de la fecha
            pfec = fin + 1;                         // Mueve el puntero al dia siguiente

            // Saca los numeros y los pasa a int/float
            if (sscanf(pmax, "%f", &dias[i].temp_max) != 1)
                break;
            if (sscanf(pmin, "%f", &dias[i].temp_min) != 1)
                break;
            if (sscanf(pllu, "%f", &dias[i].lluvia) != 1)
                break;
            if (sscanf(pcod, "%d", &dias[i].codigo_clima) != 1)
                break;

            // Avanza a la siguiente coma para medir el siguiente dia
            pmax = strchr(pmax, ',');
            if (pmax)
                pmax++;
            pmin = strchr(pmin, ',');
            if (pmin)
                pmin++;
            pllu = strchr(pllu, ',');
            if (pllu)
                pllu++;
            pcod = strchr(pcod, ',');
            if (pcod)
                pcod++;
        }

        // Printea los resultados
        for (int i = 0; i < 7; i++)
        {
            printf("  %-10s | Min: %4.0f°C Max: %4.0f°C | %-20s | Lluvia: %5.1fmm\n",
                   dias[i].fecha,
                   dias[i].temp_min,
                   dias[i].temp_max,
                   descripcion_clima(dias[i].codigo_clima),
                   dias[i].lluvia);
        }
    }

    // Todos los free necesarios para no dejar memoria colgada
    free(arr_fechas);
    free(arr_max);
    free(arr_min);
    free(arr_lluvia);
    free(arr_codigo);
    free(resp.datos);

}

void noticia_publicar()
{
    char tipo[64];
    char categoria[64];
    char titulo[256];
    char enlace[256];
    char dni_admin[32];
    char fecha_publicacion[32];

    printf("\n--- PUBLICAR NOTICIA ---\n");

    do
    {
        printf("Tipo: ");
        scanf(" %63[^\n]", tipo);
        if (strlen(tipo) == 0)
        {
            printf("[ERROR] La categoria no puede estar vacia.\n");
        }
    } while (strlen(tipo) == 0);

    do
    {
        printf("Categoria (Deportes, Cultura, Trafico, ...): ");
        scanf(" %63[^\n]", categoria);
        if (strlen(categoria) == 0)
        {
            printf("[ERROR] La categoria no puede estar vacia.\n");
        }
    } while (strlen(categoria) == 0);

    do
    {
        printf("Titulo: ");
        scanf(" %255[^\n]", titulo);
        if (strlen(titulo) == 0)
        {
            printf("[ERROR] El titulo no puede estar vacio.\n");
        }
    } while (strlen(titulo) == 0);

    do
    {
        printf("Enlace: ");
        scanf(" %255[^\n]", enlace);
        if (strlen(enlace) == 0)
        {
            printf("[ERROR] El enlace no puede estar vacio.\n");
        }

    } while (strlen(enlace) == 0);

    do
    {
        printf("DNI del admin: ");
        scanf(" %31[^\n]", dni_admin);
        limpiarBuffer();
    } while (!dni_es_valido(dni_admin));

    time_t ahora = time(NULL);
    struct tm *fecha = localtime(&ahora);
    strftime(fecha_publicacion, sizeof(fecha_publicacion), "%Y-%m-%d", fecha);

    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO Publicacion (tipo, categoria, titulo, enlace, dni_admin, fecha_publicacion, estado) "
                      "VALUES (?, ?, ?, ?, ?, ?, 'ACTIVA');";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK)
    {
        // Vinculamos cada variable al marcador '?' correspondiente
        sqlite3_bind_text(stmt, 1, tipo, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, categoria, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, titulo, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, enlace, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, dni_admin, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 6, fecha_publicacion, -1, SQLITE_STATIC);

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
void mostrarDeportes()
{
    printf("\n--- NOTICIAS DE DEPORTES ---\n");

    char *err = NULL;
    int total = 0;
    int resultado = sqlite3_exec(db,
                                 "SELECT id_publicacion, tipo, titulo, enlace, fecha_publicacion, estado "
                                 "FROM Publicacion "
                                 "WHERE categoria='Deportes' AND estado='ACTIVA';",
                                 callback_mostrar_deportes, &total, &err);

    if (resultado != SQLITE_OK)
    {
        printf("[ERROR] %s\n", err);
        sqlite3_free(err);
        return;
    }

    if (total == 0)
    {
        printf("[INFO] No hay publicaciones en la categoria Deportes.\n");
    }
}
void noticia_listar()
{
    printf("\n--- LISTADO DE LAS NOTICIAS ---\n");
    char *err = NULL;
    int total = 0;
    int resultado = sqlite3_exec(db,
                                 "SELECT id_publicacion, categoria, tipo, titulo, fecha_publicacion, estado "
                                 "FROM Publicacion;",
                                 callback_listar_noticias, &total, &err);

    if (resultado != SQLITE_OK)
    {
        printf("[ERROR] %s\n", err);
        sqlite3_free(err);
        return;
    }

    if (total == 0)
    {
        printf("[INFO] No hay noticias registradas.\n");
    }
    log_escribir("Ha consultado el listado completo de noticias");

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
            printf("1. Tipo\n");
            printf("2. Categoria\n");
            printf("3. Titulo\n");
            printf("4. Enlace\n");
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
            case 1:
                printf("Nuevo tipo: ");
                nombre_campo_sql = "tipo";
                nombre_campo_log = "tipo";
                break;
            case 2:
                printf("Nueva categoria: ");
                nombre_campo_sql = "categoria";
                nombre_campo_log = "categoria";
                break;
            case 3:
                printf("Nuevo titulo: ");
                nombre_campo_sql = "titulo";
                nombre_campo_log = "titulo";
                break;
            case 4:
                printf("Nuevo enlace: ");
                nombre_campo_sql = "enlace";
                nombre_campo_log = "enlace";
                break;
            default:
                printf("[!] Opcion invalida.\n");
                continue;
            }

            scanf(" %255[^\n]", nuevo_valor);
            limpiarBuffer();

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


