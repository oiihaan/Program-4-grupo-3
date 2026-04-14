#ifndef FUNCIONES_H
#define FUNCIONES_H

void limpiarBuffer();
char* capturar_contrasena();
int dni_es_valido(const char *dni);
int fecha_es_valida(const char *fecha);
int fecha_es_hoy_o_posterior(const char *fecha);
int comprobar_contrasena(const char *contrasena);
int obtener_entero_validado(int minimo, int maximo);
float obtener_float_validado(float minimo, float maximo);

#endif 
