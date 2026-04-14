#ifndef AUTH_H
#define AUTH_H

int auth_login();
void auth_generar_hash(const char *password, const char *fecha, char *out_hash);
void admin_registrar_nuevo();
void auth_editar_password(const char* dni);


#endif