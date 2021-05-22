CC = gcc
CFLAGS = -Wextra -g -O2 -Wall
INCLUDES = includes
SRC = src
OBJ = obj
BIN = bin

all: server client

server:	obj/aurrasd bin/aurrasd
client: obj/aurras bin/aurras

obj/aurrasd: $(SRC)/aurrasd.c $(INCLUDES)/aurrasd.h
	$(CC) $(CFLAGS) -o $(OBJ)/aurrasd.o -c $(SRC)/aurrasd.c
bin/aurrasd: $(OBJ)/aurrasd.o
	$(CC) $(CFLAGS) $(OBJ)/aurrasd.o -o $(BIN)/aurrasd

obj/aurras: $(SRC)/aurras.c $(INCLUDES)/aurras.h
	$(CC) $(CFLAGS) -o $(OBJ)/aurras.o -c $(SRC)/aurras.c 
bin/aurras: $(OBJ)/aurras.o
	$(CC) $(CFLAGS) $(OBJ)/aurras.o -o $(BIN)/aurras

clean:
	rm -rf $(OBJ)/*.o $(BIN)/{aurras,aurrasd,fifo_clientServer,fifo_serverClient}

test: 
	cd bin && ./aurra
	cd bin && ./aurras status