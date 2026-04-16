#include "../include/db.h"
#include <stdio.h>
#include "../include/log.h"


sqlite3 *db = NULL;

int db_abrir(const char *ruta) {
    if (sqlite3_open(ruta, &db) != SQLITE_OK) {
        printf("[ERROR] No se pudo abrir la BD: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    db_ejecutar("PRAGMA foreign_keys = ON;");
    printf("[OK] Base de datos abierta: %s\n", ruta);
    return 1;
}

void db_cerrar() {
    if (db) sqlite3_close(db);
    printf("[OK] Base de datos cerrada.\n");
}

int db_ejecutar(const char *sql) {
    char *err = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
        printf("[ERROR] SQL: %s\n", err);
        sqlite3_free(err);
        return 0;
    }
    return 1;
}

char* obtenerDni(char* nombre){

    
}


void db_crear_tablas() {
    db_ejecutar(
    "CREATE TABLE IF NOT EXISTS Admin ("
    "dni TEXT PRIMARY KEY,"
    "nombre_usuario TEXT NOT NULL,"
    "password TEXT NOT NULL,"
    "activo INTEGER DEFAULT 1,"
    "fecha_creacion DATETIME DEFAULT CURRENT_TIMESTAMP"
    ");"
);

    db_ejecutar(
        "CREATE TABLE IF NOT EXISTS Espacio ("
        "id_espacio INTEGER PRIMARY KEY AUTOINCREMENT,"
        "nombre TEXT NOT NULL UNIQUE,"
        "capacidad INTEGER,"
        "precio_hora REAL,"
        "activo INTEGER DEFAULT 1"
        ");"
    );

    db_ejecutar(
        "CREATE TABLE IF NOT EXISTS Reserva ("
        "id_reserva INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_espacio INTEGER,"
        "dni_ciudadano TEXT,"
        "fecha TEXT,"
        "franja_inicio TEXT,"
        "franja_fin TEXT,"
        "num_personas INTEGER,"
        "cancelada INTEGER DEFAULT 0,"
        "FOREIGN KEY (id_espacio) REFERENCES Espacio(id_espacio)"
        "ON DELETE CASCADE"
        ");"
    );

    db_ejecutar(
        "CREATE TABLE IF NOT EXISTS Publicacion ("
        "id_publicacion INTEGER PRIMARY KEY AUTOINCREMENT,"
        "categoria TEXT,"
        "titulo TEXT,"
        "enlace TEXT,"
        "dni_admin TEXT,"
        "fecha_publicacion TEXT,"
        "estado TEXT DEFAULT 'ACTIVA',"
        "FOREIGN KEY (dni_admin) REFERENCES Admin(dni)"
        ");"
    );

    db_ejecutar(
        "CREATE TABLE IF NOT EXISTS TipoLicencia ("
        "id_tipo INTEGER PRIMARY KEY AUTOINCREMENT,"
        "nombre TEXT NOT NULL,"
        "descripcion TEXT,"
        "requisitos TEXT,"
        "activo INTEGER DEFAULT 1"
        ");"
    );

    db_ejecutar(
        "CREATE TABLE IF NOT EXISTS Licencia ("
        "id_licencia INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_tipo INTEGER,"
        "dni_ciudadano TEXT,"
        "estado TEXT DEFAULT 'En revision',"
        "fecha_solicitud TEXT,"
        "fecha_expiracion TEXT,"
        "FOREIGN KEY (id_tipo) REFERENCES TipoLicencia(id_tipo)"
        "ON DELETE CASCADE"
        ");"
    );

    // QUITO ESTO PARA QUE FUNCIONE LA NUEVA FUNCION DE INICIO DE SESION,
    // ahora el programa comprueba que haya algun admin en la base de datos y si no lo hay,
    // entra en modo de "setup" para crear un admin con su clave hash.

    // // FUNCION AUXILIAR PARA HACER PRUEBAS CON LA BD
    // db_ejecutar(
    //     "INSERT OR IGNORE INTO Admin (dni, nombre_usuario, password, activo) "
    //     "VALUES ('12345678A', 'admin', '1234', 1);"
    // );
    printf("[OK] Tablas creadas/verificadas.\n");
}

void db_insertar_datos_prueba(){
    db_ejecutar(
        "INSERT OR IGNORE INTO Espacio (nombre, capacidad, precio_hora, activo) "
        "VALUES ('Sala de Conferencias', 50, 25.0, 1);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Espacio (nombre, capacidad, precio_hora, activo) "
        "VALUES ('Piscina', 20, 15.0, 1);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Espacio (nombre, capacidad, precio_hora, activo) "
        "VALUES ('Pista de padel A', 6, 10.0, 1);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Espacio (nombre, capacidad, precio_hora, activo) "
        "VALUES ('Pista de padel B', 6, 10.0, 1);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Espacio (nombre, capacidad, precio_hora, activo) "
        "VALUES ('Pista de tenis', 6, 15.0, 0);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Espacio (nombre, capacidad, precio_hora, activo) "
        "VALUES ('Pista de padel C', 6, 10.0, 1);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Espacio (nombre, capacidad, precio_hora, activo) "
        "VALUES ('Pista de tenis B', 6, 15.0, 1);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Espacio (nombre, capacidad, precio_hora, activo) "
        "VALUES ('Campo fútbol 7', 14, 35.0, 1);"
    );
    printf("[OK] Espacios de prueba creados.\n");


    db_ejecutar(
        "INSERT OR IGNORE INTO Reserva (id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin, num_personas, cancelada) "
        "VALUES (2, '22334455P', '2026-04-18', '16:00', '17:30', 8, 1);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Reserva (id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin, num_personas, cancelada) "
        "VALUES (1, '87654321Z', '2026-04-19', '09:00', '11:00', 25, 0);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Reserva (id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin, num_personas, cancelada) "
        "VALUES (2, '11223344B', '2026-04-20', '11:00', '12:30', 12, 0);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Reserva (id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin, num_personas, cancelada) "
        "VALUES (3, '55667788C', '2026-04-21', '18:30', '19:30', 4, 0);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Reserva (id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin, num_personas, cancelada) "
        "VALUES (4, '99887766D', '2026-04-22', '20:00', '21:00', 6, 1);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Reserva (id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin, num_personas, cancelada) "
        "VALUES (2, '77665544I', '2026-04-23', '14:00', '15:30', 18, 0);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Reserva (id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin, num_personas, cancelada) "
        "VALUES (5, '44556677E', '2026-04-24', '17:00', '18:00', 4, 0);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Reserva (id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin, num_personas, cancelada) "
        "VALUES (3, '88776655J', '2026-04-25', '18:00', '19:00', 6, 0);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Reserva (id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin, num_personas, cancelada) "
        "VALUES (4, '99887766K', '2026-04-26', '21:00', '22:00', 4, 1);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Reserva (id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin, num_personas, cancelada) "
        "VALUES (1, '11223344L', '2026-04-27', '10:00', '12:00', 40, 0);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Reserva (id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin, num_personas, cancelada) "
        "VALUES (6, '33445566F', '2026-04-28', '19:00', '20:00', 5, 0);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Reserva (id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin, num_personas, cancelada) "
        "VALUES (8, '55667788M', '2026-05-01', '18:00', '20:00', 14, 0);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Reserva (id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin, num_personas, cancelada) "
        "VALUES (7, '22334455G', '2026-04-30', '16:30', '17:30', 4, 1);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Reserva (id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin, num_personas, cancelada) "
        "VALUES (6, '44556677N', '2026-05-03', '17:30', '18:30', 6, 1);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Reserva (id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin, num_personas, cancelada) "
        "VALUES (7, '33445566O', '2026-05-04', '16:00', '17:00', 5, 0);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Reserva (id_espacio, dni_ciudadano, fecha, franja_inicio, franja_fin, num_personas, cancelada) "
        "VALUES (8, '66778899H', '2026-05-02', '15:00', '17:00', 12, 0);"
    );

    printf("[OK] Reservas de prueba creadas.\n");


    db_ejecutar(
        "INSERT OR IGNORE INTO TipoLicencia (nombre, descripcion, requisitos, activo) "
        "VALUES ('Licencia de Construcción', 'Para obras menores y reformas', 'Proyecto técnico, seguro', 1);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO TipoLicencia (nombre, descripcion, requisitos, activo) "
        "VALUES ('Licencia de Actividad', 'Para negocios y establecimientos', 'Documentación sanitaria, comprobante de domicilio', 1);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO TipoLicencia (nombre, descripcion, requisitos, activo) "
        "VALUES ('Licencia de Terraza', 'Para establecimientos con espacio exterior', 'Plano de ocupación, autorización propietario', 1);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO TipoLicencia (nombre, descripcion, requisitos, activo) "
        "VALUES ('Licencia de Publicidad', 'Para carteles y anuncios', 'Especificaciones técnicas, seguro de responsabilidad', 1);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO TipoLicencia (nombre, descripcion, requisitos, activo) "
        "VALUES ('Licencia de Armas', 'Para tener armas', 'Ser mayor de edad, examen psicotecnico', 1);"
    );
    printf("[OK] Tipos de licencia de prueba creados.\n");


    db_ejecutar(
        "INSERT OR IGNORE INTO Licencia (id_tipo, dni_ciudadano, estado, fecha_solicitud, fecha_expiracion) "
        "VALUES (1, '12345678B', 'En revision', '2025-04-01', '2029-04-01');"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Licencia (id_tipo, dni_ciudadano, estado, fecha_solicitud, fecha_expiracion) "
        "VALUES (2, '87654321Z', 'Aprobada', '2024-12-15', '2026-12-15');"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Licencia (id_tipo, dni_ciudadano, estado, fecha_solicitud, fecha_expiracion) "
        "VALUES (3, '11223344B', 'Denegada', '2025-02-10', 2030-02-10);"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Licencia (id_tipo, dni_ciudadano, estado, fecha_solicitud, fecha_expiracion) "
        "VALUES (4, '55667788C', 'En revision', '2025-03-20', '2028-03-20');"
    );
    db_ejecutar(
        "INSERT OR IGNORE INTO Licencia (id_tipo, dni_ciudadano, estado, fecha_solicitud, fecha_expiracion) "
        "VALUES (1, '99887766D', 'Aprobada', '2024-08-05', '2026-08-05');"
    );
    printf("[OK] Licencias de prueba creadas.\n");

    printf("*** Datos de prueba insertados correctamente ***\n\n");

    log_escribir("Se han insertado los datos de prueba en la base de datos");
}

void db_insertar_publicaciones_prueba() {
    db_ejecutar("PRAGMA foreign_keys = OFF;");

    /* --- DEPORTES --- */
    db_ejecutar(
        "INSERT INTO Publicacion (categoria, titulo, enlace, fecha_publicacion, estado) "
        "VALUES ('Deportes', 'Real Sociedad disputara la final de la Copa del Rey el 18 de abril en Sevilla', "
        "'https://www.diariovasco.com/real-sociedad/final-copa/', '2026-04-10', 'ACTIVA');"
    );
    db_ejecutar(
        "INSERT INTO Publicacion (categoria, titulo, fecha_publicacion, estado) "
        "VALUES ('Deportes', 'Baskonia golea al Barça y se acerca al liderato de la Liga ACB', '2026-04-08', 'ACTIVA');"
    );
    db_ejecutar(
        "INSERT INTO Publicacion (categoria, titulo, fecha_publicacion, estado) "
        "VALUES ('Deportes', 'Athletic Club clasifica para semifinales de la Europa League', '2026-04-05', 'ACTIVA');"
    );
    db_ejecutar(
        "INSERT INTO Publicacion (categoria, titulo, fecha_publicacion, estado) "
        "VALUES ('Deportes', 'La SD Eibar vuelve a Primera Division tras ganar el playoff de ascenso', '2026-04-03', 'ACTIVA');"
    );

    /* --- POLITICA --- */
    db_ejecutar(
        "INSERT INTO Publicacion (categoria, titulo, fecha_publicacion, estado) "
        "VALUES ('Politica', 'Imanol Pradales presenta el plan de vivienda publica para Euskadi 2026-2030', '2026-04-09', 'ACTIVA');"
    );
    db_ejecutar(
        "INSERT INTO Publicacion (categoria, titulo, fecha_publicacion, estado) "
        "VALUES ('Politica', 'El PNV y el PSE-EE renuevan el acuerdo de gobierno para toda la legislatura', '2026-04-07', 'ACTIVA');"
    );
    db_ejecutar(
        "INSERT INTO Publicacion (categoria, titulo, fecha_publicacion, estado) "
        "VALUES ('Politica', 'Pello Otxandiano propone en el Parlamento Vasco una reforma fiscal para rentas altas', '2026-04-06', 'ACTIVA');"
    );
    db_ejecutar(
        "INSERT INTO Publicacion (categoria, titulo, fecha_publicacion, estado) "
        "VALUES ('Politica', 'El Parlamento Vasco aprueba los presupuestos generales para 2026 con mayoria absoluta', '2026-04-04', 'ACTIVA');"
    );
    db_ejecutar(
        "INSERT INTO Publicacion (categoria, titulo, fecha_publicacion, estado) "
        "VALUES ('Politica', 'Arnaldo Otegi reclama el acercamiento de presos vascos en el debate sobre pacificacion', '2026-04-02', 'ACTIVA');"
    );

    db_ejecutar("PRAGMA foreign_keys = ON;");
    printf("[OK] Publicaciones de prueba creadas.\n");
}