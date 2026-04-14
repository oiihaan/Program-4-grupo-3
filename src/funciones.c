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
#include "funciones.h"

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

        unsigned char novenoCaracter = (unsigned char)dni[8];
        if (!isalpha(novenoCaracter))
        {
            printf("[ERROR] DNI invalido. El noveno caracter debe ser una letra.\n");
            return 0; // Falla la validación
        }
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