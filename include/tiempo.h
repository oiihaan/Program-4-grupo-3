#ifndef TIEMPO_H_
#define TIEMPO_H_



typedef struct {
    char fecha[16];
    float temp_max;
    float temp_min;
    float lluvia;
    int codigo_clima;
} Dia;

void mostrarTiempo();

#endif


