# Variables
CC = gcc
CXX = g++
CFLAGS = -Iinclude
CXXFLAGS = -Iinclude
LIBS = -lsqlite3 -lcurl

# Archivos comunes (db, log, auth, sha256, utils, funciones)
C_COMMON = src/auth.c src/db.c src/funciones.c src/log.c src/sa256.c src/utils.c

# Admin: archivos comunes + módulos de admin
ADMIN_C   = $(C_COMMON) $(wildcard admin/*.c)
ADMIN_CPP = src/main.cpp
ADMIN_OBJ = $(ADMIN_C:.c=.o) $(ADMIN_CPP:.cpp=.o)

# Servidor
SERVER_C   = $(C_COMMON) admin/config.c
SERVER_CPP = server/server.cpp
SERVER_OBJ = $(SERVER_C:.c=.o) $(SERVER_CPP:.cpp=.o)

# Cliente
CLIENT_C   = src/funciones.c src/log.c
CLIENT_CPP = cliente/cliente.cpp cliente/main_cliente.cpp
CLIENT_OBJ = $(CLIENT_C:.c=.o) $(CLIENT_CPP:.cpp=.o)

BUILD_DIR = build
SERVER_PID_FILE = .server.pid

# Reglas
all: $(BUILD_DIR) build/main.exe build/servidor.exe build/cliente.exe

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/main.exe: $(ADMIN_OBJ)
	$(CXX) $(ADMIN_OBJ) $(LIBS) -o $@

build/servidor.exe: $(SERVER_OBJ)
	$(CXX) $(SERVER_OBJ) $(LIBS) -o $@

build/cliente.exe: $(CLIENT_OBJ)
	$(CXX) $(CLIENT_OBJ) $(LIBS) -o $@

run: build/main.exe
	./build/main.exe

run-server: build/servidor.exe build/main.exe
	@echo "Iniciando servidor en segundo plano..."
	@./build/servidor.exe & echo $$! > $(SERVER_PID_FILE)
	@sleep 1
	@./build/main.exe
	@echo "Admin cerrado. Servidor sigue en segundo plano (usa 'make stop-server' para pararlo)."

run-cliente: build/cliente.exe
	./build/cliente.exe

stop-server:
	@if [ -f $(SERVER_PID_FILE) ]; then \
		PID=$$(cat $(SERVER_PID_FILE)); \
		if kill -0 $$PID 2>/dev/null; then \
			kill $$PID && echo "Servidor detenido (PID=$$PID)."; \
		else \
			echo "No hay servidor activo con PID=$$PID (PID file obsoleto)."; \
		fi; \
		rm -f $(SERVER_PID_FILE); \
	else \
		echo "No existe $(SERVER_PID_FILE). Intentando detener por nombre..."; \
		pkill -x servidor.exe >/dev/null 2>&1 && echo "Servidor detenido por nombre." || echo "No se encontró servidor en ejecución."; \
	fi

clean:
	rm -f src/*.o admin/*.o server/*.o cliente/*.o build/*.exe

.PHONY: all run run-server run-cliente stop-server clean
