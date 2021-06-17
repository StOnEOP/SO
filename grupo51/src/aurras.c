#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define MAX 2048

//Teste
// ./bin/aurras transform samples/sample-1-so.m4a tmp/sample-1.m4a alto

size_t readln(int fd, char* line, size_t size) {
    size_t bytes_read = read(fd, line, size);
    if(!bytes_read) return 0;

    size_t line_length = strcspn(line, "\n") + 1;
    if(bytes_read < line_length) line_length = bytes_read;
    line[line_length] = 0;
    
    lseek(fd, line_length - bytes_read, SEEK_CUR);
    return line_length;
}

int transformInput(char *arg) {
    if (strcasecmp(arg, "transform") == 0)
        return 1;
    else
        if (strcasecmp(arg, "status") == 0)
            return 2;
        else
            if (strlen(arg) > 5) {
                char *configFileChecker = strchr(arg, '.');
                if (configFileChecker != NULL) {
                    configFileChecker++;
                    if (strcmp(configFileChecker, "conf") == 0)
                        return 3;
                }
            }

    return 0;
}

int main(int argc, char *argv[]) {
    char toServer[MAX], fromServer[MAX];
    char pipeName[50];
    char pid[50];
    
    sprintf(pid,"%d", getpid());
    sprintf(pipeName, "tmp/fifo_%d", getpid());
    mkfifo(pipeName, 0644);
    
    if (argc < 2) {     // INFO
        int fifo_connection = open("tmp/fifo_connection", O_WRONLY);

        if(write(fifo_connection, pipeName, strlen(pipeName)) < 0){
            perror("Erro na conexão\n");
            return -1;
        }
        close(fifo_connection);

        int fifo_clientServer = open(pipeName, O_WRONLY);
        
        strcpy(toServer, "ajuda");
        if (write(fifo_clientServer, toServer, strlen(toServer)) < 0) {
            perror("Erro\n\tEscrita no fifo clientServer\n");
            return -1;
        }
        close(fifo_clientServer);

        int fifo_serverClient = open(pipeName, O_RDONLY);
        int bytesread = 0;
        if ((bytesread = read(fifo_serverClient, fromServer, MAX)) > 0)
            if (write(STDOUT_FILENO, fromServer, bytesread) < 0) {
                perror("Erro\n\tEscrita no stdout\n");
                return -1;
            }
        close(fifo_serverClient);
    }

    else {
        int caseS = 0;
        caseS = transformInput(argv[1]);

        switch(caseS) {
            case 1:     // Transform (1)
                strcpy(toServer, "transform ");
                int c1 = 2;
                while (c1 < argc) {
                    strcat(toServer, argv[c1++]);
                    if (c1 < argc)
                        strcat(toServer, " ");
                }
                break;

            case 2:     // Status (2)
                strcpy(toServer, "status");
                break;

            default:
                strcpy(toServer, "0:");
                break;
        }

        int fifo_connection = open("tmp/fifo_connection", O_WRONLY);

        if(write(fifo_connection, pipeName, strlen(pipeName)) < 0){
            perror("Erro na conexão\n");
            return -1;
        }
        close(fifo_connection);

        int fifo_clientServer = open(pipeName, O_WRONLY);

        if (write(fifo_clientServer, toServer, strlen(toServer)) < 0) {
            perror("Erro\n\t- escrita no fifo clientServer\n");
            return -1;
        }
        close(fifo_clientServer);

        char* token;
        char* fromBuffer = NULL;
        int bytesread = 0;
        int writedbytes = 0;
        int fifo_serverClient = open(pipeName, O_RDONLY);
        fromServer[0] = '\0';

        while((bytesread = read(fifo_serverClient, fromServer, MAX)) > 0){
            fromBuffer = strdup(fromServer);
            while ((token = strtok_r(fromBuffer, "\n", &fromBuffer))){
                write(STDOUT_FILENO, "\n", 2);
                if (strcmp(token, pid) == 0){
                        break;
                }
                if ((writedbytes = write(STDOUT_FILENO, token, strlen(token))) < 0) {
                    perror("Erro\n\t- escrita no stdout\n");
                    return -1;
                }
            }
        }
        write(STDOUT_FILENO, "\n", 2);
        close(fifo_serverClient);
    }

    return 0;
}