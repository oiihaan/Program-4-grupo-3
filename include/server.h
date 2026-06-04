#ifndef SERVER_H
#define SERVER_H

#define SERVER_PID_FILE ".server.pid"

#ifdef __cplusplus
#include <string>
namespace server {
class Server {
private:
    int puerto;
    int estado;

public:
    Server(int puerto);
    ~Server();

    int getEstado() const;
    void setEstado(int estado);
    int getPuerto() const;
    void setPuerto(int puerto);
};
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

void server_configurar_desde_puerto(int puerto);
int server_get_puerto(void);
void server_set_estado(int estado);
int server_get_estado(void);
void server_verificar_estado(void);
int server_probar_puerto(int puerto);

#ifdef __cplusplus
}
#endif

#endif
