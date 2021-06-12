#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>

#define MAX 2048

//Teste
// ./bin/aurrasd etc/aurrasd.conf bin/aurrasd-filter

/*
TODO 
- Cliente so pode acabar quando o ficheiro estiver pronto!
- Quando estao os 2 filtros a ser utilizados o status nao funciona (demora).
- Fica em loop no CheckFilterUsage quando corre o terceiro processo (limite do filtro é 2). Faltas decrementar filtros no sigchld_handler

Updates:
Pus o exec a receber o que é suposto (temos muitas linhas de codigo que agora ja nao sao precisas, mas depois
limpa-se).
O que penso que seja preciso fazer é um dup2 na linha 300 (antes do fork) em que redirecionamos o stdin
para o ficheiro audio que é dado como input e o stdout para o ficheiro que é dado para output do programa.
Nao tenho a certeza mas penso que é isso.
No fim nao esquecer voltar a por os file descriptors normais. (Ex guiao4 ex 3, e ex 5).
O processo filho mantem os mesmos file descriptors que o pai e o exec tambem por isso deveria funcionar.
Apos o redirecionamento penso que o programa ja funcione para inputs em que é dado apenas um filtro.
Depois disto é corrigir os erros acima no TODO e tentar fazer a cena dos pipes para recebermos mais do
que um filtro.

*/

struct filter {
    char name[50]; 
    char exec_path[100];
    char exec [100];
    int total;
    int usage;
};

int terminate = 0;
int task_number = 0;
char* task_status[2048];
char* task_command[2048];
int empty_array = 1;
struct filter filter_array[100];

size_t readln(int fd, char* line, size_t size) {
    size_t bytes_read = read(fd, line, size);
    if(!bytes_read) return 0;

    size_t line_length = strcspn(line, "\n") + 1;
    if(bytes_read < line_length) line_length = bytes_read;
    line[line_length] = 0;
    
    lseek(fd, line_length - bytes_read, SEEK_CUR);
    return line_length;
}

char* getExec(char* token) {
    for(int i= 0; filter_array[i].name[0]; i++)
        if(strcmp(filter_array[i].name, token) == 0)
            return filter_array[i].exec;
    return "ERRO";
}

char* getExecPath(char* token) {
    for(int i= 0; filter_array[i].name[0]; i++)
        if(strcmp(filter_array[i].name, token) == 0)
            return filter_array[i].exec_path;
    return "ERRO";
}

int checkFiltersUsage(char* filters) {
    char* token;
    while ((token = strtok_r(filters, " ", &filters))){
        for (int i = 0; filter_array[i].name[0]; i++){
            if (strcmp(filter_array[i].name, token) == 0){
                if (filter_array[i].usage == filter_array[i].total){
                    printf("Usage %d, Total %d\n", filter_array[i].usage, filter_array[i].total);
                    return 1;
                }
            }
        }
    }
    return 0;
}

void incrementFilters(char* filters) {
    char* token;
    while ((token = strtok_r(filters, " ", &filters)))
        for (int i = 0; filter_array[i].name[0]; i++)
            if (strcmp(filter_array[i].name, token) == 0)
                filter_array[i].usage++;
}

void decrementFilters(char* filters) {
    char* token;
    while ((token = strtok_r(filters, " ", &filters)))
        for (int i = 0; filter_array[i].name[0]; i++)
            if (strcmp(filter_array[i].name, token) == 0)
                filter_array[i].usage--;
}

void sigchld_handler(int sig) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
        for (int i = 0; i <= task_number-1; i++)
            if (WEXITSTATUS(status) == 1) {
                kill(pid, SIGTERM);
                //decrementfilters task i
                task_status[i] = "ERROR";
                break;
            }
            else if (strcmp(task_status[i], "EXECUTING") == 0){
                kill(pid, SIGTERM);
                //decrementfilters task i
                task_status[i] = "FINISHED";
            }
}

void sigterm_handler(int sig) {
    terminate = 1;
    int tasks_terminated = 0;
    int counter = 0;

    while (!tasks_terminated) {
        counter = 0;
        for (int i = 0; i<= task_number; i++)
            if (strcmp(task_status[i], "EXECUTING") == 0)
                counter++;
        if (!counter)
            tasks_terminated = 1;
        //sleep(3);
    }
}

int main(int argc, char *argv[]) {
    //Configuring server
    if(argc != 3) {
        perror("Invalid arguments to start the server\n");
        return 1;
    }

    char path[1024];
    char line[1024];
    char* line2;
    char* bar = "/";
    char* token;
    int counter = 0;
    int counter2 = 0;
    ssize_t bytes_read = 0;

    strcpy(path, argv[1]);
    //strcat(path, bar);
    //strcat(path, argv[1]);
    printf("Path %s\n", path);
    int fd = open(path, O_RDONLY);
    if(fd == -1) {
        perror("Error opening config file\n");
        return 1;
    }
    else {
        while ((bytes_read = readln(fd, line, 1024))) {
            //printf("%s\n", line);
            line2 = strdup(line);
            while ((token = strtok_r(line2, " ", &line2))) {
                //printf("Token %s\n", token);
                if (counter2 == 0)
                    strcpy(filter_array[counter].name, token);
                if (counter2 == 1) {
                    strcpy(filter_array[counter].exec_path, "bin/aurrasd-filters/");
                    strcpy(filter_array[counter].exec, token);
                    strcat(filter_array[counter].exec_path, token);
                }
                if (counter2 == 2)
                    filter_array[counter].total = atoi(token);
                counter2++;
            }
            filter_array[counter].usage = 0;
            counter2 = 0;
            counter++;
        }
    }
    for (int i = 0; i<100 && filter_array[i].name[0]; i++) {
        printf("Name- %s\n", filter_array[i].name);
        printf("Exec- %s\n", filter_array[i].exec_path);
        printf("Total- %d\n", filter_array[i].total);
        printf("Usage- %d\n", filter_array[i].usage);
    }

    mkfifo("fifo_clientServer", 0644);
    mkfifo("fifo_serverClient", 0644);

    signal(SIGCHLD, sigchld_handler);
    signal(SIGTERM, sigterm_handler);

    //char **log = malloc(sizeof(char*) * 1);
    //int iLog = 0;
    while (!terminate) {
        char *buffer = malloc(sizeof(char) * MAX);
        int fifo_clientServer = open("fifo_clientServer", O_RDONLY);
        int fifo_serverClient = open("fifo_serverClient", O_WRONLY);

        if (read(fifo_clientServer, buffer, MAX) < 0)
            strcpy(buffer, "Erro");
            
        if (strncmp(buffer, "ajuda", 5) == 0) { // Listar a maneira correta de utilizar o programa
            char comandos[] = "\nComandos diponiveis:\n\n"
                                "./aurras status\n"
                                "./aurras transform input-filename output-filename filter-id-1 filter-id-2...\n\n";
            write(fifo_serverClient, comandos, strlen(comandos));
        }
        else if (strncmp(buffer, "transform", 9) == 0) { // ffmpeg -i input.mp3 -filter "filtro_1, filtro_2" output.mp3
            int pipes[32][2];
            int currentPipe = 0;
            char message [64];
            char* args [1024];
            char* token;
            char* filter_names = NULL;
            char * exec_names = NULL;
            char* straux = NULL;
            char* straux2 = NULL;
            int counter = 0;
            int fst = 0;
            int fst2 = 0;
            char* v = ", ";
            char* v2 = " ";

            task_command[task_number] = strdup(buffer);
            task_status[task_number] = "EXECUTING";

            while ((token = strtok_r(buffer, " ", &buffer))) {
                printf("BUF %s \n", token);
                if (counter == 3) {
                    if (!fst2) {
                        fst2 = 1;
                        filter_names = strdup(token);
                        straux = strdup(getExecPath(token));
                        straux2 = strdup(getExec(token));
                    }   
                    else {
                        strcat(straux, getExec(token));
                        strcat(filter_names, token);
                    }
                    strcat(straux, v);
                    strcat(straux2, v);
                    strcat(filter_names, v2);
                }
                else {
                    if (fst)
                        args[counter-1] = strdup(token);  // ffmpeg -i input.mp3 output.mp3 filtro_1 filtro_2
                    fst = 1;
                    counter++;
                }
            }

            straux[strlen(straux)-2] = '\0';
            straux2[strlen(straux2)-2] = '\0';
            //args[0] = straux;
            counter++;
            printf("Filter_names %s \n", filter_names);
            printf("STrAux %s \n", straux);
            printf("STrAux2 %s \n", straux2);
            //token = args[3];
            //args[3] = "-filter";
            //args[counter-1] = token;    // ffmpeg -i input.mp3 -filter "filtro_1 filtro_2" output.mp3
            printf("BBB\n");
            for(int i = 0; args[i]; i++)
                printf("Args[%d] - %s\n", i, args[i]);

            int fst_time = 0;
            //fazer uma função checkfilter a correr num while para so continuar quando houver filtros disponiveis
            char* filter_names2 = strdup(filter_names);
            printf("FILTER NAMES - %s\n", filter_names);
            while (checkFiltersUsage(filter_names)) {
                if(!fst_time) {
                    fst_time = 1;
                    sprintf(message, "\nPending task #%d\n\n", task_number+1);
                    write(fifo_serverClient, message, strlen(message));
                }
                printf("Cheguei \n");
                sleep(3);
            }

            //Incrementar os filtros que vao ser usados
            incrementFilters(filter_names2);

            sprintf(message, "\nProcessing task #%d\n\n", task_number+1);
            write(fifo_serverClient, message, strlen(message));

            int stdin_faudio = open(args[0], O_RDONLY);
            int stdout_faudio = open(args[1], O_WRONLY | O_CREAT | O_TRUNC);

            int i = 0, pid;
            char *token2 = NULL;
            while ((token2 = strtok_r(filter_names, " ", &filter_names))) {
                if (i == 0) {
                    pipe(pipes[currentPipe]);
                    if ((pid = fork()) == 0) {
                        close(pipes[currentPipe][0]);
                        dup2(stdin_faudio, STDIN_FILENO);
                        dup2(pipes[currentPipe][1], STDOUT_FILENO);
                        close(stdin_faudio);
                        close(pipes[currentPipe][1]);
                        execl(getExecPath(token), getExec(token), NULL);
                        exit(1);
                    }
                }
                else {
                    pipe(pipes[currentPipe]);
                    if ((pid = fork()) == 0) {
                        close(pipes[currentPipe][0]);
                        if (currentPipe != 0) {
                            dup2(pipes[currentPipe-1][0], STDIN_FILENO);
                            close(pipes[currentPipe-1][0]);
                        }
                        dup2(pipes[currentPipe][1], STDOUT_FILENO);
                        close(pipes[currentPipe][1]);
                        execl(getExecPath(token), getExec(token), NULL);
                        exit(1);
                    }
                }

                close(pipes[currentPipe][1]);
                if (currentPipe != 0)
                    close(pipes[currentPipe-1][0]);
                currentPipe++;
                i++;
            }
            if ((pid = fork()) == 0) {
                dup2(stdout_faudio, STDOUT_FILENO);
                close(stdout_faudio);
                if (currentPipe != 0) {
                    dup2(pipes[currentPipe-1][0], STDIN_FILENO);
                    close(pipes[currentPipe-1][0]);
                }
                execl(getExecPath(token), getExec(path), NULL);
                exit(1);
            }

            task_number++;
        }

        else if (strncmp(buffer, "status", 6) == 0) {
            char message[1024];
            int empty = 1;
            strcpy(message, "\nTasks Executing:\n");
            write(fifo_serverClient, message, strlen(message));

            for (int i = 0; i < task_number; i++)
                if (strcmp(task_status[i], "EXECUTING") == 0) {
                    empty = 0;
                    sprintf(message, "task #%d: %s\n", i+1, task_command[i]);
                    write(fifo_serverClient, message, strlen(message));
                }
            for (int i = 0; filter_array[i].name[0]; i++) {
                sprintf(message, "filter %s: %d/%d (running/max)\n", filter_array[i].name, filter_array[i].usage, filter_array[i].total);
                write(fifo_serverClient, message, strlen(message));
            }
            sprintf(message, "pid: %d\n", getpid());
            write(fifo_serverClient, message, strlen(message));
            write(fifo_serverClient, "\n", 1);
            if (empty) {
                sprintf(message, "There are no tasks executing\n");
                write(fifo_serverClient, message, strlen(message));
            }
        }
    }
    return 0;
}