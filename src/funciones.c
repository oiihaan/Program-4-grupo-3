#include "../include/funciones.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "../include/espacios.h"
#include "../include/noticias.h"
#include "../include/licencias.h"
#include "../include/reservas.h"
// estas las he pillado que estaban abajo revisar si sirven de algo
#include <termios.h>
#include <unistd.h>

void limpiarBuffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
    {
    }
}

char *capturar_contrasena()
{
    struct termios viejo, nuevo;
    char *password = NULL;
    int i = 0;
    int ch;

    // Configuración de la terminal: desactivamos el eco (ECHO)
    tcgetattr(STDIN_FILENO, &viejo);
    nuevo = viejo;
    nuevo.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &nuevo);

    printf("Introduce tu clave: ");

    while (1)
    {
        ch = getchar();

        if (ch == '\n' || ch == '\r')
        { // Enter
            break;
        }
        else if (ch == 127 || ch == 8)
        { // Backspace (Borrar)
            if (i > 0)
            {
                i--;
                printf("\b \b"); // Mueve el cursor atrás, imprime espacio, vuelve atrás
                password = (char *)realloc(password, (i + 1) * sizeof(char));
            }
        }
        else
        {
            // Reservamos espacio para la nueva letra + el hueco del '\0'
            char *temp = (char *)realloc(password, (i + 2) * sizeof(char));
            if (temp == NULL)
            {
                free(password);
                return NULL;
            }
            password = temp;
            password[i++] = (char)ch;
            printf("*"); // El "engaño" visual
        }
    }

    if (password != NULL)
    {
        password[i] = '\0'; // Cerramos la cadena
    }

    // Restauramos la consola a su estado original
    tcsetattr(STDIN_FILENO, TCSANOW, &viejo);
    printf("\n");

    return password; // ¡Recuerda hacer free() de esto en el main!
}

int dni_es_valido(const char *dni)
{
    static const char *letras = "TRWAGMYFPDXBNJZSQVHLCKE";
    if (!dni || strlen(dni) != 9)
    {
        printf("[ERROR] DNI invalido. Formato esperado: 12345678Z.\n");
        return 0;
    }

    int numero = 0;
    for (int i = 0; i < 8; i++)
    {
        if (!isdigit((unsigned char)dni[i]))
        {
            printf("[ERROR] DNI invalido. Los primeros 8 caracteres deben ser numeros.\n");
            return 0;
        }
        numero = numero * 10 + (dni[i] - '0');
    }

    unsigned char novenoCaracter = (unsigned char)dni[8];
    if (!isalpha(novenoCaracter))
    {
        printf("[ERROR] DNI invalido. El noveno caracter debe ser una letra.\n");
        return 0;
    }

    char letra_correcta = letras[numero % 23];
    if (toupper(novenoCaracter) != letra_correcta)
    {
        printf("[ERROR] DNI invalido. La letra no corresponde al numero (deberia ser %c).\n",
               letra_correcta);
        return 0;
    }

    return 1;
}

int fecha_es_valida(const char *fecha)
{
    if (!fecha || strlen(fecha) != 10)
    {
        printf("[ERROR] Formato de fecha invalido. Tiene que ser YYYY-MM-DD.\n");
        return 0;
    }
    if (fecha[4] != '-' || fecha[7] != '-')
    {
        printf("[ERROR] Formato de fecha invalido. Tiene que ser YYYY-MM-DD.\n");
        return 0;
    }

    for (int i = 0; i < 10; i++)
    {
        if (i == 4 || i == 7)
            continue;
        if (!isdigit((unsigned char)fecha[i]))
        {
            printf("[ERROR] Formato de fecha invalido. Tiene que ser YYYY-MM-DD.\n");
            return 0;
        }
    }

    int anio = 0, mes = 0, dia = 0;
    if (sscanf(fecha, "%4d-%2d-%2d", &anio, &mes, &dia) != 3)
    {
        printf("[ERROR] Formato de fecha invalido. Tiene que ser YYYY-MM-DD.\n");
        return 0;
    }
    if (mes < 1 || mes > 12 || dia < 1)
    {
        printf("[ERROR] Fecha con valores fuera de rango.\n");
        return 0;
    }

    int dias_mes[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int bisiesto = ((anio % 4 == 0 && anio % 100 != 0) || (anio % 400 == 0));
    if (bisiesto)
        dias_mes[1] = 29;

    if (dia > dias_mes[mes - 1])
    {
        printf("[ERROR] El dia %d no existe en el mes %d.\n", dia, mes);
        return 0;
    }

    return 1;
}

int fecha_es_hoy_o_posterior(const char *fecha)
{
    int anio = 0, mes = 0, dia = 0;
    if (sscanf(fecha, "%4d-%2d-%2d", &anio, &mes, &dia) != 3)
    {
        printf("[ERROR] Formato de fecha invalido. Tiene que ser YYYY-MM-DD.\n");
        return 0;
    }

    time_t t = time(NULL);
    struct tm hoy = *localtime(&t);

    if (anio < (hoy.tm_year + 1900) ||
        (anio == (hoy.tm_year + 1900) && mes < (hoy.tm_mon + 1)) ||
        (anio == (hoy.tm_year + 1900) && mes == (hoy.tm_mon + 1) && dia < hoy.tm_mday))
    {
        printf("[ERROR] La fecha no puede ser anterior a hoy (%04d-%02d-%02d).\n",
               hoy.tm_year + 1900, hoy.tm_mon + 1, hoy.tm_mday);
        return 0;
    }

    return 1;
}
int comprobar_contrasena(const char *password) {
    if (!password || password == NULL || strlen(password) == 0) {
        printf("[ERROR] La contraseña no puede estar vacia.\n");
        return 0;
    }
    return 1;
}

int obtener_entero_validado(int minimo, int maximo)
{
    int valor;
    int valido = 0;

    while (!valido) {
        if (scanf("%d", &valor) != 1) {
            limpiarBuffer();
            printf("[ERROR] Entrada invalida. Por favor, introduce un numero entero.\n");
            continue;
        }

        if (valor < minimo || valor > maximo) {
            printf("[ERROR] El valor debe estar entre %d y %d.\n", minimo, maximo);
            continue;
        }

        valido = 1;
    }

    limpiarBuffer();
    return valor;
}

float obtener_float_validado(float minimo, float maximo)
{
    float valor;
    int valido = 0;

    while (!valido) {
        if (scanf("%f", &valor) != 1) {
            limpiarBuffer();
            printf("[ERROR] Entrada invalida. Por favor, introduce un numero decimal.\n");
            continue;
        }

        if (valor < minimo || valor > maximo) {
            printf("[ERROR] El valor debe estar entre %.2f y %.2f.\n", minimo, maximo);
            continue;
        }

        valido = 1;
    }

    limpiarBuffer();
    return valor;
}

/* ── mostrarTiempo — consulta API meteorológica (curl, sin BD) ── */
#include <curl/curl.h>
#include "../include/noticias.h"   /* typedef Dia */


#define URL_TIEMPO_FC \
    "https://api.open-meteo.com/v1/forecast?latitude=43.3128&longitude=-1.975" \
    "&daily=weather_code,temperature_2m_max,temperature_2m_min,rain_sum" \
    "&timezone=Europe%2FBerlin"

typedef struct { char *datos; size_t tam; } RespuestaCurl;

static size_t callback_curl_fc(void *contenido, size_t tam, size_t nmemb, RespuestaCurl *resp)
{
    size_t total = tam * nmemb;
    resp->datos = realloc(resp->datos, resp->tam + total + 1);
    if (!resp->datos) return 0;
    memcpy(resp->datos + resp->tam, contenido, total);
    resp->tam += total;
    resp->datos[resp->tam] = '\0';
    return total;
}

static char *extraer_array_fc(const char *json, const char *campo)
{
    char buscar[64];
    snprintf(buscar, sizeof(buscar), "\"%s\":[", campo);
    char *ini = strstr(json, buscar);
    if (!ini) return NULL;
    ini = strchr(ini, '[') + 1;
    char *fin = strchr(ini, ']');
    if (!fin) return NULL;
    size_t len = fin - ini;
    char *resultado = malloc(len + 1);
    strncpy(resultado, ini, len);
    resultado[len] = '\0';
    return resultado;
}

static const char *descripcion_clima_fc(int codigo)
{
    if (codigo == 0)  return "Despejado";
    if (codigo <= 3)  return "Parcialmente nublado";
    if (codigo <= 48) return "Niebla";
    if (codigo <= 57) return "Llovizna";
    if (codigo <= 67) return "Lluvia";
    if (codigo <= 77) return "Nieve";
    if (codigo <= 82) return "Chubascos";
    if (codigo <= 99) return "Tormenta";
    return "Desconocido";
}

void mostrarTiempo()
{
    printf("\n--- TIEMPO EN DONOSTI (proximos 7 dias) ---\n");
    CURL *curl = curl_easy_init();
    if (!curl) { printf("[ERROR] No se pudo inicializar curl.\n"); return; }

    RespuestaCurl resp;
    resp.datos = malloc(1);
    resp.tam   = 0;
    resp.datos[0] = '\0';

    curl_easy_setopt(curl, CURLOPT_URL,           URL_TIEMPO_FC);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback_curl_fc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &resp);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,       10L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        printf("[ERROR] No se pudo obtener el tiempo: %s\n", curl_easy_strerror(res));
        free(resp.datos); return;
    }
    if (strstr(resp.datos, "<html>") != NULL) {
        printf("[ERROR] El servidor de la API no esta disponible. Intentalo mas tarde.\n");
        free(resp.datos); return;
    }

    char *arr_fechas = extraer_array_fc(resp.datos, "time");
    char *arr_max    = extraer_array_fc(resp.datos, "temperature_2m_max");
    char *arr_min    = extraer_array_fc(resp.datos, "temperature_2m_min");
    char *arr_lluvia = extraer_array_fc(resp.datos, "rain_sum");
    char *arr_codigo = extraer_array_fc(resp.datos, "weather_code");

    if (!arr_fechas || !arr_max || !arr_min || !arr_lluvia || !arr_codigo) {
        printf("[ERROR] No se pudo parsear la respuesta.\n");
    } else {
        Dia dias[7];
        char *pfec = arr_fechas, *pmax = arr_max, *pmin = arr_min;
        char *pllu = arr_lluvia, *pcod = arr_codigo;

        for (int i = 0; i < 7; i++) {
            char *ini = strchr(pfec, '"');
            if (!ini) break;
            ini++;
            char *fin = strchr(ini, '"');
            if (!fin) break;
            strncpy(dias[i].fecha, ini, fin - ini);
            dias[i].fecha[fin - ini] = '\0';
            pfec = fin + 1;

            if (sscanf(pmax, "%f", &dias[i].temp_max) != 1) break;
            if (sscanf(pmin, "%f", &dias[i].temp_min) != 1) break;
            if (sscanf(pllu, "%f", &dias[i].lluvia)   != 1) break;
            if (sscanf(pcod, "%d", &dias[i].codigo_clima) != 1) break;

            pmax = strchr(pmax, ','); if (pmax) pmax++;
            pmin = strchr(pmin, ','); if (pmin) pmin++;
            pllu = strchr(pllu, ','); if (pllu) pllu++;
            pcod = strchr(pcod, ','); if (pcod) pcod++;
        }

        for (int i = 0; i < 7; i++) {
            printf("  %-10s | Min: %4.0f\xc2\xb0""C Max: %4.0f\xc2\xb0""C | %-20s | Lluvia: %5.1fmm\n",
                   dias[i].fecha, dias[i].temp_min, dias[i].temp_max,
                   descripcion_clima_fc(dias[i].codigo_clima), dias[i].lluvia);
        }
    }

    free(arr_fechas); free(arr_max); free(arr_min);
    free(arr_lluvia); free(arr_codigo); free(resp.datos);
}
