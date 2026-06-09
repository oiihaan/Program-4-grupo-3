#ifndef CLIENTE_H
#define CLIENTE_H

namespace cliente {

    /* ── Clase base: datos comunes a cualquier usuario registrado ── */
    class Usuario {
    protected:
        int         id;
        char       *nombre;
        char       *dni;

    public:
        Usuario(int id, const char *nombre, const char *dni);
        Usuario(const Usuario &otro);
        virtual ~Usuario();

        int         getId()     const;
        const char *getNombre() const;
        const char *getDni()    const;

        /* Polimorfismo: cada subclase decide qué contenido puede ver */
        virtual bool puedeVerPolitica() const = 0;   /* clase abstracta */
        virtual const char *getTipo()   const = 0;
    };

    /* ── Cliente mayor de edad (>= 18 años) ── */
    class ClienteMayor : public Usuario {
    public:
        ClienteMayor(int id, const char *nombre, const char *dni);
        ClienteMayor(const ClienteMayor &otro);
        ~ClienteMayor() override;

        bool        puedeVerPolitica() const override { return true;  }
        const char *getTipo()          const override { return "Mayor de edad"; }
    };

    /* ── Cliente menor de edad (< 18 años) ── */
    class ClienteMenor : public Usuario {
    public:
        ClienteMenor(int id, const char *nombre, const char *dni);
        ClienteMenor(const ClienteMenor &otro);
        ~ClienteMenor() override;

        bool        puedeVerPolitica() const override { return false; }
        const char *getTipo()          const override { return "Menor de edad"; }
    };

}  /* namespace cliente */

/* ── Funciones C (sockets, login, registro) ── */
#ifdef __cplusplus
extern "C" {
#endif

int   cliente_conectar(const char *ip, const char *port);
int   cliente_enviar_recibir(int sockfd, const char *comando, char *respuesta, int max_size);
void  cliente_cerrar(int sockfd);
void  cliente_set_socket(int sockfd);
int   cliente_login(int *out_es_mayor);   /* devuelve 1=OK; llena out_es_mayor */
void  cliente_registrar_nuevo();
int   cliente_intentos_agotados();
const char *cliente_get_nombre_sesion(void);
void  cliente_cambiar_password(int sockfd);

#ifdef __cplusplus
}
#endif

#endif /* CLIENTE_H */
