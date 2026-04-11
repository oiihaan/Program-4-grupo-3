# Variables para que sea más fácil de leer
CC = gcc
CFLAGS = -Iinclude $(SQLITE_CFLAGS) $(CURL_CFLAGS)
LIBS = $(SQLITE_LIBS) $(CURL_LIBS)
#SRC = src/auth.c src/config.c src/db.c src/espacios.c src/funciones.c src/main.c src/noticias.c src/log.c
SRC = $(wildcard src/*.c)
OUT = build/main.exe
BUILD_DIR = build
SQLITE_LIBS ?= -lsqlite3
CURL_LIBS ?= -lcurl

# Regla principal
all: $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(OUT)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Regla para ejecutar (el famoso make run)
run: all
	./$(OUT)

# Regla para limpiar los archivos generados
clean:
	rm -f $(OUT)

.PHONY: all run clean
