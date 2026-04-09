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

Al ejecutar por primera vez, el sistema creará automáticamente:
- La base de datos (`ayuntamiento.db`) con todas las tablas necesarias
- Un usuario administrador de prueba
- Un fichero de log (`log.txt`) donde se registran todas las acciones

**Credenciales por defecto:**

| Campo    | Valor  |
|----------|--------|
| Usuario  | admin  |
| Contraseña | 1234 |

---

## Funcionalidades

- **Gestión de espacios** — Añadir, listar, eliminar y cambiar estado de espacios municipales
- **Reservas** — Crear, listar, editar y cancelar reservas de espacios
- **Noticias y alertas** — Publicar, editar y eliminar publicaciones por categoría ademas de la prevision meteorologica
- **Licencias y permisos** — Registrar expedientes, gestionar estados y tipos de licencia
- **Log del sistema** — Registro automático de todas las acciones en `log.txt`

---

## Estructura del proyecto

```
Program-4-grupo-3/
├── include/          # Cabeceras .h
├── src/              # Código fuente .c
├── build/            # Ejecutable generado
├── datos/            # Base de datos SQLite
├── Makefile
├── server.conf       # Configuración del servidor
└── log.txt           # Registro de actividad (se crea al ejecutar)
```

---

## Notas

- La API del tiempo utiliza [Open-Meteo](https://open-meteo.com/) — servicio gratuito sin necesidad de registro. Requiere conexión a internet. Si el servidor está caído, el sistema lo notificará con un mensaje de error.
- La base de datos y el log se generan automáticamente en el primer arranque, no es necesario crearlos manualmente.
- El sistema registra en `log.txt` cada acción realizada, incluyendo inicios de sesión, modificaciones y errores de autenticación.
