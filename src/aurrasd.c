#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../includes/aurrasd.h"
#define MAX 2048

int main(int argc, char *argv[]) {
    mkfifo("fifo_clientServer", 0644);
    mkfifo("fifo_serverClient", 0644);

    while (1) {
        char *buffer = malloc(sizeof(char) * MAX);
        int fifo_clientServer = open("fifo_clientServer", O_RDONLY);
        int fifo_serverClient = open("fifo_serverClient", O_WRONLY);

        if (read(fifo_clientServer, buffer, MAX) < 0)
            strcpy(buffer, "Erro");
        
        printf("Server: %s\n", buffer);
        free(buffer);

        close(fifo_clientServer);
        close(fifo_serverClient);
    }

    return 0;
}