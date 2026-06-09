#ifndef NOTICIAS_H
#define NOTICIAS_H

/* Las funciones de consulta (verNoticias, mostrarDeportes,
   mostrarPolitica, noticia_listar, mostrarTiempo) se declaran
   en funciones.h y se implementan en src/funciones.c           */

typedef struct {
    char fecha[16];
    float temp_max;
    float temp_min;
    float lluvia;
    int codigo_clima;
} Dia;

/* ── Funciones exclusivas del modulo admin de noticias ── */
void submenuNoticias();
void noticia_publicar();
void noticia_gestionar(); /* Menu para editar o eliminar noticias */

#endif // NOTICIAS_H
