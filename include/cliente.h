#ifndef CLIENTE_H
#define CLIENTE_H

namespace cliente {
    class Cliente {
    private:
        int id;
        int edad;
        char* nombre_cliente;
        char* contrasena;
        char* licencias;

    public:
        Cliente(int id, int edad, const char* nombre_cliente, const char* contrasena, const char* licencias);
        Cliente(const Cliente& otro);
        ~Cliente();

        int getId() const;
        int getEdad() const;
        const char* getNombreCliente() const;
        const char* getContrasena() const;
        const char* getLicencias() const;
    };
}

#ifdef __cplusplus
extern "C" {
#endif

int cliente_conectar(const char *ip, const char *port);
int cliente_enviar_recibir(int sockfd, const char *comando, char *respuesta, int max_size);
void cliente_cerrar(int sockfd);
void cliente_set_socket(int sockfd);
int cliente_login();
void cliente_registrar_nuevo();
int cliente_intentos_agotados();
const char* cliente_get_nombre_sesion(void);
void cliente_cambiar_password(int sockfd);


#ifdef __cplusplus
}
#endif

#endif