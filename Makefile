CC = gcc
CFLAGS = -std=c99 -Wextra -g -O2 -Wall
INCLUDES = includes
SRC = src
OBJ = obj
BIN = bin

all: server client

server:	obj/aurrasd aurrasd
client: obj/aurras aurras

obj/aurrasd: $(SRC)/aurrasd.c $(INCLUDES)/aurrasd.h
	$(CC) $(CFLAGS) -o $(OBJ)/aurrasd.o -c $(SRC)/aurrasd.c
aurrasd: $(OBJ)/aurrasd.o
	$(CC) $(CFLAGS) $(OBJ)/aurrasd.o -o $(BIN)/aurrasd

obj/aurras: $(SRC)/aurras.c $(INCLUDES)/aurras.h
	$(CC) $(CFLAGS) -o $(OBJ)/aurras.o -c $(SRC)/aurras.c 
aurras: $(OBJ)/aurras.o
	$(CC) $(CFLAGS) $(OBJ)/aurras.o -o $(BIN)/aurras

clean:
	rm -rf $(OBJ)/*.o $(BIN)/{aurrasd, aurras}