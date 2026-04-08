// include/config.h
#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    char db_ruta[256];
    char admin_usuario[64];
    char server_puerto[16];
} Config;

extern Config config;

int config_cargar(const char *ruta);
void config_mostrar();
int definir_intentos();

#endif