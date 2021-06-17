all: server client

server: obj/aurrasd.o bin/aurrasd

client: obj/aurras.o bin/aurras

obj/aurrasd.o: src/aurrasd.c
	gcc -Wall -g -c src/aurrasd.c -o obj/aurrasd.o

bin/aurras: obj/aurras.o
	gcc -g obj/aurras.o -o bin/aurras

bin/aurrasd: obj/aurrasd.o
	gcc -g obj/aurrasd.o -o bin/aurrasd

obj/aurras.o: src/aurras.c
	gcc -Wall -g -c src/aurras.c -o obj/aurras.o

clean:
	rm obj/* tmp/* bin/aurras bin/aurrasd

test:
	bin/aurras transform samples/sample-1-so.m4a tmp/sample-1.mp3 alto
