# Ayuntamiento de Donosti — Sistema de Gestión Municipal
**Programación IV — Grupo 3**  
Oihan Saez de Cortazar, Pablo Gonzalez, Danel Pozo, Unai Garcia, Markel Vesga y Jurgi Hernandez

---

## Descripción
Aplicación de consola en C para la gestión interna del Ayuntamiento de Donostia-San Sebastián. Permite administrar espacios públicos y reservas, publicar noticias y alertas, gestionar licencias y permisos, y consultar el tiempo de la próxima semana mediante una API externa.

---

## Requisitos previos
Antes de compilar, instala las siguientes librerías en tu sistema Linux / WSL:
```bash
sudo apt install libsqlite3-dev
sudo apt install libcurl4-openssl-dev
```
> El proyecto está desarrollado y probado sobre **WSL (Windows Subsystem for Linux)** con Ubuntu 24.

---

## Compilación y ejecución
El proyecto incluye un **Makefile**. Desde la raíz del proyecto ejecuta:
```bash
make run
```
Esto compilará el proyecto automáticamente y lo ejecutará. Para compilar sin ejecutar:
```bash
make
```
Para limpiar los archivos compilados:
```bash
make clean
```

---

## Primer inicio
Al primer arranque del sistema, se iniciará automáticamente el modo de configuración inicial (setup), en el que se solicitarán las credenciales necesarias para crear el administrador principal.

Nota: Este proceso solo se ejecuta una única vez, ya sea en el primer lanzamiento del programa o tras la eliminación de la base de datos. 

---

## Seguridad y Criptografía
El sistema implementa un esquema de seguridad robusto para la gestión de identidades:

- **Hashing de contraseñas:** Las contraseñas nunca se almacenan en texto plano. Se utiliza el algoritmo SHA-256 para generar un resumen irreversible.
- **Salting dinámico:** Para evitar ataques de diccionario, se utiliza la columna `fecha_creacion` de cada usuario como "sal". Esto garantiza que, aunque dos administradores usen la misma contraseña, sus hashes en la base de datos sean completamente diferentes.
- **Implementación:** La lógica criptográfica utiliza la librería [TinyCrypt (Intel)](https://github.com/intel/tinycrypt/tree/master).
- **Intentos de login:** El sistema limita los intentos de autenticación fallidos consecutivos y cierra el programa tras superarse el límite.

---

## Funcionalidades

- **Gestión de espacios** — Añadir, listar, eliminar y cambiar estado (ACTIVO / BAJA) de espacios municipales
- **Reservas** — Consultar reservas por espacio, crear y cancelar reservas de ciudadanos
- **Noticias** — Publicar, editar y eliminar publicaciones por categoría, además de previsión meteorológica
- **Licencias** — Registrar licencia, administrar tipos de licencia, actualizar estados y consultar expedientes
- **Configuración** — Gestión de usuarios y contraseñas de administradores
- **Log del sistema** — Registro automático de todas las acciones en `log.txt`, incluyendo inicios de sesión y errores de autenticación
- **Prevención de errores** — Validación de formato y rangos de valores en todas las entradas del usuario

---

## Base de datos
El sistema gestiona las siguientes tablas en SQLite:

| Tabla          | Descripción                                     |
|----------------|-------------------------------------------------|
| `Admin`        | Administradores del sistema                     |
| `Espacio`      | Espacios municipales disponibles                |
| `Reserva`      | Reservas de ciudadanos sobre espacios           |
| `Publicacion`  | Publicaciones                                   |
| `TipoLicencia` | Tipos de licencia disponibles                   |
| `Licencia`     | Expedientes de licencias de ciudadanos          |

---

## Estructura del proyecto
```
Program-4-grupo-3/
├── include/          # Cabeceras .h
├── src/              # Código fuente .c
│   ├── main.c
│   ├── auth.c
│   ├── config.c
│   ├── db.c
│   ├── espacios.c
│   ├── funciones.c
│   └── licencias.c (+ otros módulos)
├── build/            # Ejecutable generado
├── Makefile
├── server.conf       # Configuración del servidor (BD, usuario, puerto)
├── log.txt           # Registro de actividad (se crea al ejecutar)
└──ayuntamiento.db    # Base de datos del sistema (se crea al ejecutar)
```

---

## Configuración (`server.conf`)
El fichero `server.conf` permite ajustar parámetros básicos del sistema:
```
db_ruta=./ayuntamiento.db
admin_usuario=admin
server_puerto=8080
max_intentos=5
hora_apertura=09:00
hora_cierre=21:00

```

---

## Notas
- La API del tiempo utiliza [Open-Meteo](https://open-meteo.com/) — servicio gratuito sin necesidad de registro. Requiere conexión a internet. Si el servidor está caído, el sistema lo notificará con un mensaje de error.
- La base de datos y el log se generan automáticamente en el primer arranque, no es necesario crearlos manualmente.
- El proyecto usa Git; consulta [.gitignore] para archivos excluidos de control de versiones.
