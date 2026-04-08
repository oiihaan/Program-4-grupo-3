#ifndef NOTICIAS_H
#define NOTICIAS_H

void verNoticias();

typedef struct {
    char fecha[16];
    float temp_max;
    float temp_min;
    float lluvia;
    int codigo_clima;
} Dia;

void mostrarTiempo();
void noticia_publicar();
void mostrarDeportes();
void noticia_eliminar();
void noticia_listar(); //Muestra todas las noticias, incluso las eliminadas

#endif // NOTICIAS_H