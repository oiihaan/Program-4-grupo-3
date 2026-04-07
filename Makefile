# Variables para que sea más fácil de leer
CC = gcc
CFLAGS = -Iinclude
LIBS = -lsqlite3 -lcurl
SRC = src/auth.c src/config.c src/db.c src/espacios.c src/funciones.c src/main.c src/tiempo.c src/log.c
OUT = build/main.exe

# Regla principal
all:
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(OUT)

# Regla para ejecutar (el famoso make run)
run: all
	./$(OUT)

# Regla para limpiar los archivos generados
clean:
	rm -f $(OUT)