# Variables para que sea más fácil de leer
CC = gcc
CXX = g++
CFLAGS = -Iinclude $(SQLITE_CFLAGS) $(CURL_CFLAGS)
CXXFLAGS = -Iinclude $(SQLITE_CFLAGS) $(CURL_CFLAGS)
LIBS = $(SQLITE_LIBS) $(CURL_LIBS)
C_SRC = $(wildcard src/*.c)
CPP_SRC = $(wildcard src/*.cpp)
C_OBJ = $(C_SRC:.c=.o)
CPP_OBJ = $(CPP_SRC:.cpp=.o)
OUT = build/main.exe
BUILD_DIR = build
SQLITE_LIBS ?= -lsqlite3
CURL_LIBS ?= -lcurl

# Regla principal
all: $(BUILD_DIR) $(OUT)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compilar archivos .c con gcc
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compilar archivos .cpp con g++
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Linkear todo con g++
$(OUT): $(C_OBJ) $(CPP_OBJ)
	$(CXX) $(C_OBJ) $(CPP_OBJ) $(LIBS) -o $(OUT)

# Regla para ejecutar (el famoso make run)
run: all
	./$(OUT)

# Regla para limpiar los archivos generados
clean:
	rm -f src/*.o $(OUT)

.PHONY: all run clean
