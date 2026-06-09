# Ayuntamiento de Donosti — Sistema de Gestión Municipal
**Programación IV — Grupo 3**  
Oihan Saez de Cortazar, Pablo Gonzalez, Danel Pozo, Unai Garcia, Markel Vesga y Jurgi Hernandez

---

## Descripción
Aplicación de consola en C/C++ para la gestión interna del Ayuntamiento de Donostia-San Sebastián.  
El sistema sigue una **arquitectura cliente-servidor TCP**: un servidor central gestiona la base de datos y atiende peticiones tanto del panel de administración como de clientes ciudadanos conectados en red.

Funcionalidades principales:
- Administrar espacios públicos y reservas
- Publicar, editar y eliminar noticias y alertas por categoría
- Gestionar licencias y permisos municipales
- Consultar la previsión meteorológica de la próxima semana (API externa)
- Gestión de usuarios administradores y autenticación segura

---

## Requisitos previos
Instala las siguientes librerías en tu sistema Linux / WSL antes de compilar:
```bash
sudo apt install libsqlite3-dev
sudo apt install libcurl4-openssl-dev
```
> El proyecto está desarrollado y probado sobre **WSL (Windows Subsystem for Linux)** con Ubuntu 24.

---

## Compilación
El proyecto incluye un **Makefile**. Desde la raíz del proyecto ejecuta:
```bash
make        # Compila los tres ejecutables (main.exe, servidor.exe, cliente.exe)
make clean  # Elimina todos los objetos y ejecutables generados
```

---

## Modos de ejecución

El sistema genera **tres ejecutables** independientes en `build/`:

### 1. Panel de administración (`main.exe`) — punto de entrada principal
Interfaz de consola para el administrador municipal. **El servidor TCP se arranca y detiene desde dentro del propio panel**, en la sección de configuración. No es necesario lanzarlo manualmente.
```bash
make run
# o bien directamente:
./build/main.exe
```

### 2. Cliente ciudadano (`cliente.exe`)
Interfaz de consola para ciudadanos que se conectan al servidor mediante TCP. Requiere que el servidor esté activo (iniciado previamente desde el panel de administración).
```bash
make run-cliente
# o bien directamente:
./build/cliente.exe
```

### Targets auxiliares del Makefile
`make run-server` y `make stop-server` son atajos de línea de comandos para arrancar/detener el servidor sin pasar por el panel. Están pensados para pruebas o entornos de desarrollo, no son el flujo de uso habitual.
```bash
make run-server   # Arranca el servidor en segundo plano y lanza el panel
make stop-server  # Detiene el servidor manualmente
```

---

## Primer inicio
Al primer arranque del sistema se ejecuta automáticamente el modo de **configuración inicial (setup)**, en el que se solicitan las credenciales para crear el administrador principal.

> Este proceso ocurre una única vez: en el primer lanzamiento o tras eliminar la base de datos.

---

## Tests
El proyecto incluye una suite de tests unitarios e integración:
```bash
make test
```
Los tests cubren los módulos `funciones`, `auth`, `db` y las funciones de tiempo. Requieren las mismas dependencias que la compilación principal.

---

## Seguridad y Criptografía
El sistema implementa un esquema robusto para la gestión de identidades:

- **Hashing de contraseñas:** Las contraseñas nunca se almacenan en texto plano. Se utiliza SHA-256 para generar un resumen irreversible.
- **Salting dinámico:** Se usa la columna `fecha_creacion` de cada usuario como "sal", garantizando que dos administradores con la misma contraseña tengan hashes distintos en la base de datos.
- **Implementación criptográfica:** Basada en la librería [TinyCrypt (Intel)](https://github.com/intel/tinycrypt/tree/master).
- **Intentos de login:** El sistema limita los intentos fallidos consecutivos y cierra el programa al superarse el límite (configurable en `server.conf`).

---

## Funcionalidades

- **Gestión de espacios** — Añadir, listar, eliminar y cambiar estado (ACTIVO / BAJA) de espacios municipales
- **Reservas** — Consultar reservas por espacio, crear y cancelar reservas de ciudadanos
- **Noticias** — Publicar, editar y eliminar publicaciones por categoría, además de previsión meteorológica
- **Licencias** — Registrar licencias, administrar tipos, actualizar estados y consultar expedientes
- **Configuración** — Gestión de usuarios y contraseñas de administradores; arranque/parada del servidor TCP desde el panel
- **Log del sistema** — Registro automático de todas las acciones en `log.txt`, incluyendo inicios de sesión y errores de autenticación
- **Prevención de errores** — Validación de formato y rangos en todas las entradas del usuario

---

## Base de datos
El sistema gestiona las siguientes tablas en SQLite:

| Tabla          | Descripción                                     |
|----------------|-------------------------------------------------|
| `Admin`        | Administradores del sistema                     |
| `Espacio`      | Espacios municipales disponibles                |
| `Reserva`      | Reservas de ciudadanos sobre espacios           |
| `Publicacion`  | Noticias y publicaciones municipales            |
| `TipoLicencia` | Tipos de licencia disponibles                   |
| `Licencia`     | Expedientes de licencias de ciudadanos          |

---

## Estructura del proyecto
```
Program-4-grupo-3/
├── include/              # Cabeceras .h
├── src/                  # Módulos comunes (C)
│   ├── main.cpp          # Panel de administración
│   ├── auth.c
│   ├── db.c
│   ├── funciones.c
│   ├── log.c
│   ├── noticias_consulta.c
│   ├── sa256.c
│   └── utils.c
├── admin/                # Módulos exclusivos del panel admin (C)
│   ├── config.c
│   ├── espacios.c
│   ├── licencias.c
│   ├── noticias.c
│   └── reservas.c
├── server/               # Servidor TCP (C++)
│   └── server.cpp
├── cliente/              # Cliente ciudadano (C++)
│   ├── cliente.cpp
│   └── main_cliente.cpp
├── tests/                # Tests unitarios e integración
│   ├── test_framework.h
│   ├── test_auth.c
│   ├── test_db.c
│   ├── test_funciones.c
│   ├── test_tiempo.c
│   ├── stubs.c
│   └── stubs_db.c
├── build/                # Ejecutables generados
│   ├── main.exe
│   ├── servidor.exe
│   └── cliente.exe
├── Makefile
├── server.conf           # Configuración del servidor
├── log.txt               # Registro de actividad (se crea al ejecutar)
└── ayuntamiento.db       # Base de datos SQLite (se crea al ejecutar)
```

---

## Configuración (`server.conf`)
El fichero `server.conf` permite ajustar los parámetros básicos del sistema:
```
db_ruta=./ayuntamiento.db
admin_usuario=admin
server_puerto=8080
max_intentos=3
hora_apertura=09:00
hora_cierre=21:00
```

| Parámetro        | Descripción                                              |
|------------------|----------------------------------------------------------|
| `db_ruta`        | Ruta a la base de datos SQLite                          |
| `admin_usuario`  | Nombre de usuario del administrador por defecto          |
| `server_puerto`  | Puerto TCP en el que escucha el servidor                 |
| `max_intentos`   | Intentos de login fallidos antes de bloquear el acceso   |
| `hora_apertura`  | Hora de inicio del servicio al ciudadano                 |
| `hora_cierre`    | Hora de cierre del servicio al ciudadano                 |

---

## Notas
- La API del tiempo utiliza [Open-Meteo](https://open-meteo.com/) — servicio gratuito sin registro. Requiere conexión a internet. Si el servidor está caído, el sistema lo notifica con un mensaje de error.
- La base de datos y el log se generan automáticamente en el primer arranque; no es necesario crearlos manualmente.
- El proyecto usa Git; consulta [`.gitignore`](.gitignore) para los archivos excluidos del control de versiones.
