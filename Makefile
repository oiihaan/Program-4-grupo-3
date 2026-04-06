CC = gcc
SRCS = $(wildcard src/*.c)
FLAGS = -lsqlite3
OUT = build/main.exe

all:
	$(CC) $(SRCS) $(FLAGS) -o $(OUT)

run: all
	./$(OUT)

clean:
	rm -f $(OUT)