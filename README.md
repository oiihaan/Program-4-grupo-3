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
Al ejecutar por primera vez, el sistema cargará la configuración desde `server.conf` y creará automáticamente la base de datos `ayuntamiento.db` con las tablas necesarias y datos de ejemplo. Si no existe ningún administrador, el sistema lo notificará en el arranque.

**Credenciales por defecto:**
| Campo      | Valor  |
|------------|--------|
| Usuario    | admin  |
| Contraseña | 1234   |

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
- **Noticias y alertas** — Publicar, editar y eliminar publicaciones por tipo y categoría, además de previsión meteorológica
- **Licencias y permisos** — Registrar expedientes, gestionar tipos de licencia, cambiar estados y consultar expedientes
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
| `Publicacion`  | Noticias, alertas y publicaciones               |
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
└── log.txt           # Registro de actividad (se crea al ejecutar)
```

---

## Configuración (`server.conf`)
El fichero `server.conf` permite ajustar parámetros básicos del sistema:
```
db_ruta=datos/ayuntamiento.db
admin_usuario=admin
server_puerto=8080
```

---

## Notas
- La API del tiempo utiliza [Open-Meteo](https://open-meteo.com/) — servicio gratuito sin necesidad de registro. Requiere conexión a internet. Si el servidor está caído, el sistema lo notificará con un mensaje de error.
- La base de datos y el log se generan automáticamente en el primer arranque, no es necesario crearlos manualmente.
- En cualquier momento se puede introducir `0` para volver atrás en los menús.
