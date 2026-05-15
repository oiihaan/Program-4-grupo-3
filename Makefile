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

run-server: build/servidor.exe
	./build/servidor.exe

run-cliente: build/cliente.exe
	./build/cliente.exe

clean:
	rm -f src/*.o admin/*.o server/*.o cliente/*.o build/*.exe

.PHONY: all run run-server run-cliente clean