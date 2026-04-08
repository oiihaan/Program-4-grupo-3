#ifndef RESERVAS_H
#define RESERVAS_H

// Funciones CRUD para gestión de reservas
void reservas_crear();                      // CREATE - Crear nueva reserva
void reservas_listar_todas();              // READ - Ver todas las reservas
void reservas_listar_por_espacio();        // READ - Ver reservas de un espacio específico
void reservas_editar();                    // UPDATE - Editar reserva existente
void reservas_cancelar();                  // UPDATE - Cancelar una reserva

#endif
