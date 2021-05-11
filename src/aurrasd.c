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


    char **log = malloc(sizeof(char*) * 1);
    int iLog = 0;
    while (1) {
        char *buffer = malloc(sizeof(char) * MAX);
        int fifo_clientServer = open("fifo_clientServer", O_RDONLY);
        int fifo_serverClient = open("fifo_serverClient", O_WRONLY);

        if (read(fifo_clientServer, buffer, MAX) < 0)
            strcpy(buffer, "Erro");
        buffer[strlen(buffer)] = '\0';
        close(fifo_clientServer);

        if (buffer != NULL) {
            char *bufferCPY = strcpy(bufferCPY, buffer);
            int option = atoi(strtok(buffer, ":"));

            char *logAUX = NULL;
            switch (option) {
                case 1: ;    // Transform (1)
                    char *logString = malloc(sizeof(char) * MAX);

                    logAUX = strtok(bufferCPY, "-");
                    logAUX = strtok(NULL, "-");
                    logString = strcpy(logString, logAUX);
                    logAUX = strtok(NULL, "-");
                    if (logAUX)
                        logAUX[strlen(logAUX)-1] = '\0';

                    while (logAUX) {
                        logString = strcat(logString, logAUX);
                        logString = strcat(logString, " ");
                        logAUX = strtok(NULL, "-");
                        if (logAUX)
                            logAUX[strlen(logAUX)] = '\0';
                    }
                    logString[strlen(logString)-1] = '\0';

                    log[iLog] = malloc(sizeof(char) * (strlen(logString) + 50));
                    sprintf(log[iLog++], "%s %s %s\n", "./aurras", "transform", logString);
                    log = realloc(log, (iLog+1) * sizeof(char*));

                    // Falta realmente fazer a transform

                    free(logString);
                    break;
                case 2:     // Status (2)
                    log[iLog] = malloc(sizeof(char) * 50);
                    sprintf(log[iLog++], "%s %s\n", "./aurras", "status");
                    log = realloc(log, (iLog+1) * sizeof(char*));

                    // Criar matriz que guarda as tarefas (transform) e imprimir as que estão em funcionamento
                    // Imprimir os filtros com o numero em utilização e o máximo
                    // Imprimir o pid

                    break;
                case 3:     // Exec server (3)
                    // Adicionar ao log. No máximo 2 argumentos? (2 ficheiros)

                    break;
                case 4:     // Info (4)
                    if (write(fifo_serverClient, "--- Informação de utilização ---\n", 38) < 0) {
                        perror("Erro\n\tInformação de utilização\n\tEscrita no fifo serverClient\n");
                        break;
                    }

                    for (int i = 0; i < iLog && log[i]; i++)
                        if (write(fifo_serverClient, log[i], strlen(log[i])) < 0) {
                            perror("--- Erro ---\n\tInformação de utilização\n\tEscrita no fifo serverClient\n");
                            break;
                        }

                    break;
                default:
                    if (write(fifo_serverClient, "--- Erro ---\n\t- Argumento inválido\n", 29) < 0)
                        perror("--- Erro ---\n\tDefault\n\tEscrita no fifo serverClient\n");

                    break;
            }
        }
        free(buffer);

        close(fifo_serverClient);
    }
    log[iLog] = NULL;

    for (int i = 0; i < iLog && log[i]; i++)
        free(log[i]);
    free(log);

    return 0;
}