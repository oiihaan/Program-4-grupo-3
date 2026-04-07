#include "../include/tiempo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

//API de Open-Meteo
#define URL_TIEMPO "https://api.open-meteo.com/v1/forecast?latitude=43.3128&longitude=-1.975&daily=weather_code,temperature_2m_max,temperature_2m_min,rain_sum&timezone=Europe%2FBerlin"

// Buffer dinamico para acumular la respuesta HTTP
typedef struct {
    char *datos;
    size_t tam; //Pongo long por si devuelve una cantidad enorme de info aunque es poco probable
} Respuesta; //Todo lo que devuelve la API en Json


//Funcion para reservar el espacio necesario para la respuesta que nos manda la API 
static size_t callback_curl(void *contenido, size_t tam, size_t nmemb, Respuesta *resp) {
    size_t total = tam * nmemb; //Tamaño total (bytes)
    //Añade mas espacio al ya reservado
    resp->datos = realloc(resp->datos, resp->tam + total + 1);
    //Si se acaba la RAM, medida de seguridad de Claude
    if (!resp->datos) return 0;
    //Recibe los datos nuevos y los añade justo donde acababa antes
    memcpy(resp->datos + resp->tam, contenido, total);
    //Guarda el nuevo tamaño
    resp->tam += total;
    //Caracter nulo para que el ordenador sepa donde acaba el array
    resp->datos[resp->tam] = '\0';
    return total;
}


//Separa los datos del Json, como si leyera un CSV
static char *extraer_array(const char *json, const char *campo) {

    char buscar[64];
    snprintf(buscar, sizeof(buscar), "\"%s\":[", campo); //Para buscar a que campo pertenece

    char *ini = strstr(json, buscar);
    if (!ini){ 
        return NULL;
    }

    ini = strchr(ini, '[') + 1; //Busca donde abre el corchete y empieza nueva info
    char *fin = strchr(ini, ']'); //Busca donde haya un corchete, asi corta el Json
    if (!fin){ 
        return NULL;
    }

    //Calcular cuantos caracteres hay en medio
    size_t len = fin - ini;

    //Reserva espacio para el resultado
    char *resultado = malloc(len + 1); //+1 para caracter nulo

    //Solamente copia ini a resultado (StringCopy)
    //La n es porque se indica la longitud
    strncpy(resultado, ini, len);

    //Cierra el String
    resultado[len] = '\0';
    
    return resultado;
    //Hacer FREE luego 
}


//Traduce el numero de codigoClima que nos da la web y lo traduce en palabras
//Esta funcion de es Claude entera
static const char *descripcion_clima(int codigo) {
    if (codigo == 0)        return "Despejado";
    if (codigo <= 3)        return "Parcialmente nublado";
    if (codigo <= 48)       return "Niebla";
    if (codigo <= 57)       return "Llovizna";
    if (codigo <= 67)       return "Lluvia";
    if (codigo <= 77)       return "Nieve";
    if (codigo <= 82)       return "Chubascos";
    if (codigo <= 99)       return "Tormenta";
    return "Desconocido";
}


//Mostrar el Tiempo
void mostrarTiempo() {
    printf("\n--- TIEMPO EN DONOSTI (proximos 7 dias) ---\n");

    // Inicializar curl
    CURL *curl = curl_easy_init();

    if (!curl) { //Prevencion de errores
        printf("[ERROR] No se pudo inicializar curl.\n");
        return;
    }

    //Giuardo el espacio para guardar la respuesta
    Respuesta resp;
    resp.datos = malloc(1); //Empieza con espacio minimo y luego añade mas segun la funcion de antes
    resp.tam = 0;
    resp.datos[0] = '\0';

    // Configurar curl
    curl_easy_setopt(curl, CURLOPT_URL, URL_TIEMPO);                //URL de la API
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback_curl);   //Escribe los datos
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);               //Indica donde escribirlos
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);                   //Tiempo de espera de 10 segundos

    // Hacer la peticion (Descarga los datos)
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl); //Cerramos el motor 

    if (res != CURLE_OK) { //Prevencion de errores
        printf("[ERROR] No se pudo obtener el tiempo: %s\n", curl_easy_strerror(res));
        free(resp.datos);
        return;
    }

    // Comprobar que la respuesta es JSON y no un error HTML
    if (strstr(resp.datos, "<html>") != NULL) {
        printf("[ERROR] El servidor de la API no esta disponible. Intentalo mas tarde.\n");
        free(resp.datos);
        return;
    }

    // Extraer cada array del JSON con la funcion de antes
    char *arr_fechas  = extraer_array(resp.datos, "time");
    char *arr_max     = extraer_array(resp.datos, "temperature_2m_max");
    char *arr_min     = extraer_array(resp.datos, "temperature_2m_min");
    char *arr_lluvia  = extraer_array(resp.datos, "rain_sum");
    char *arr_codigo  = extraer_array(resp.datos, "weather_code");

    //Si hay algun dato que no aparece se corta
    if (!arr_fechas || !arr_max || !arr_min || !arr_lluvia || !arr_codigo) {
        printf("[ERROR] No se pudo parsear la respuesta.\n");
    }
    
     else { //Si todo esta correcto...
        // Rellenar array de structs Dia
        Dia dias[7];

        //Estos punteros son para recorrer la lista sin perder el inicio (Claude)
        char *pfec = arr_fechas;
        char *pmax = arr_max;
        char *pmin = arr_min;
        char *pllu = arr_lluvia;
        char *pcod = arr_codigo;

        
        for (int i = 0; i < 7; i++) {
            // La fecha esta entre comillas
            char *ini = strchr(pfec, '"'); //Primera comilla (Como corchetes antes)
            
            if (!ini){ 
                break;
            }

            ini++;
            char *fin = strchr(ini, '"'); //Segunda comilla (cierre)

            if (!fin){ 
                break;
            }

            strncpy(dias[i].fecha, ini, fin - ini); //Copia el trozo de la fecha
            dias[i].fecha[fin - ini] = '\0'; //Cierra el String de la fecha
            pfec = fin + 1; //Mueve el puntero al dia siguiente

            //Saca los numeros y los pasa a int/float
            if (sscanf(pmax, "%f", &dias[i].temp_max) != 1) break;
            if (sscanf(pmin, "%f", &dias[i].temp_min) != 1) break;
            if (sscanf(pllu, "%f", &dias[i].lluvia)   != 1) break;
            if (sscanf(pcod, "%d", &dias[i].codigo_clima) != 1) break;

            //Avanza a la siguiente coma para medir el siguiente dia
            pmax = strchr(pmax, ','); if (pmax) pmax++;
            pmin = strchr(pmin, ','); if (pmin) pmin++;
            pllu = strchr(pllu, ','); if (pllu) pllu++;
            pcod = strchr(pcod, ','); if (pcod) pcod++;
        }

        //Printea los resultados
        for (int i = 0; i < 7; i++) {
            printf("  %s  |  Min: %.0f°C  Max: %.0f°C  |  %-20s  |  Lluvia: %.1fmm\n",
                dias[i].fecha,
                dias[i].temp_min,
                dias[i].temp_max,
                descripcion_clima(dias[i].codigo_clima),
                dias[i].lluvia
            );
        }
    }

    //Todos los free necesarios para no dejar memoria colgada
    free(arr_fechas); 
    free(arr_max); 
    free(arr_min);
    free(arr_lluvia); 
    free(arr_codigo);

    free(resp.datos);
}