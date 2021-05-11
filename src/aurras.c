#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>

#include "../includes/aurras.h"
#define MAX 2048

int transformInput(char *arg) {
    if (strcasecmp(arg, "transform") == 0)
        return 1;
    else
        if (strcasecmp(arg, "status") == 0)
            return 2;
        else {
            if (strlen(arg) > 5) {
                char *configFileChecker = strchr(arg, '.');
                if (configFileChecker != NULL) {
                    configFileChecker++;
                    if (strcmp(configFileChecker, "conf") == 0)
                        return 3;
                }
            }
        }

    return 0;
}

int main(int argc, char *argv[]) {
    char toServer[MAX], fromServer[MAX];
    
    if (argc < 2) {     // INFO
        int fifo_clientServer = open("fifo_clientServer", O_WRONLY);
        int fifo_serverClient = open("fifo_serverClient", O_RDONLY);
        
        strcpy(toServer, "info");
        if (write(fifo_clientServer, toServer, strlen(toServer)) < 0) {
            perror("Erro\n\tEscrita no fifo clientServer\n");
            close(fifo_clientServer);
            close(fifo_serverClient);
            return -1;
        }
        close(fifo_clientServer);

        int bytesread = 0;
        while ((bytesread = read(fifo_serverClient, fromServer, MAX)) > 0)
            if (write(STDOUT_FILENO, fromServer, bytesread) < 0) {
                perror("Erro\n\tEscrita no stdout\n");
                close(fifo_serverClient);
                return -1;
            }
        close(fifo_serverClient);
    }
    else {
        int caseS = 0;
        caseS = transformInput(argv[1]);

        switch(caseS) {
            case 1:     // Transform
                strcpy(toServer, "transform -");
                int c1 = 2;
                while (c1 < argc) {
                    strcat(toServer, argv[c1++]);
                    if (c1 < argc)
                        strcat(toServer, " -");
                }
                break;
            case 2:     // Status
                strcpy(toServer, "status");
                break;
            case 3:     // Exec server
                strcpy(toServer, "exec -");
                int c3 = 1;
                while (c3 < argc) {
                    strcat(toServer, argv[c3++]);
                    if (c3 < argc)
                        strcat(toServer, " -");
                }
                break;
            default:
                strcpy(toServer, "default");
                break;
        }
        int fifo_clientServer = open("fifo_clientServer", O_WRONLY);
        int fifo_serverClient = open("fifo_serverClient", O_RDONLY);

        if (write(fifo_clientServer, toServer, strlen(toServer)) < 0) {
            perror("Erro\n\t- escrita no fifo clientServer\n");
            close(fifo_clientServer);
            close(fifo_serverClient);
            return -1;
        }
        close(fifo_clientServer);

        int bytesread = 0;
        while ((bytesread = read(fifo_serverClient, fromServer, MAX)) > 0)
            if (write(STDOUT_FILENO, fromServer, bytesread) < 0) {
                perror("Erro\n\t- escrita no stdout\n");
                close(fifo_serverClient);
                return -1;
            }
        close(fifo_serverClient);
    }

    return 0;
}