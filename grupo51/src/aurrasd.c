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
char* clientPids[2048];
int total_childs[2048];
int pids[2048][32];
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

void endTask(int i){
    for(int j = 0; j < total_childs[i]; j++){
        kill(pids[i][j], SIGTERM);
    }
}

char* getExec(char* token) {
    for(int i= 0; filter_array[i].name[0]; i++){
        if(strcmp(filter_array[i].name, token) == 0)
            return filter_array[i].exec;
    }
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
    int j = 0;
    printf("Decrementing\n");
    while ((token = strtok_r(filters, " ", &filters))){
        if(j > 2){
            printf("TOken to decrement - %s\n", token);
            for (int i = 0; filter_array[i].name[0]; i++)
                if (strcmp(filter_array[i].name, token) == 0)
                    filter_array[i].usage--;
        }
        j++;
    }
}

void sigchld_handler(int sig) {
    int status;
    pid_t pid;
    printf("Child died\n");
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0){
        for (int i = 0; i <= task_number-1; i++){
            for(int j = 0; j < total_childs[i]; j++){
                if (pids[i][j] == pid && WEXITSTATUS(status) == 1) {
                    endTask(i);
                    printf("Child Error\n");
                    decrementFilters(task_command[i]);
                    task_status[i] = "ERROR";
                    break;
                }
            } // Se o ultimo filho criado para uma certa task tiver terminado e o status da mesma for executing significa que terminou com sucesso
            if (pids[i][total_childs[i] - 1] == pid && strcmp(task_status[i], "EXECUTING") == 0){ 
                decrementFilters(task_command[i]);
                task_status[i] = "FINISHED";
                char fifopath[100];
                sprintf(fifopath, "tmp/fifo_%s", clientPids[i]);
                int fifo_clientServer = open(fifopath, O_WRONLY);
                write(fifo_clientServer, clientPids[i], strlen(clientPids[i]));
                close(fifo_clientServer);
            }
        }
    }
}

void sigterm_handler(int sig) {
    terminate = 1;
    int tasks_terminated = 0;
    int counter = 0;

    while (!tasks_terminated) {
        counter = 0;
        for (int i = 0; i<= task_number-1; i++)
            if (strcmp(task_status[i], "EXECUTING") == 0)
                counter++;
        if (!counter)
            tasks_terminated = 1;
    }
}


int main(int argc, char *argv[]) {

    signal(SIGCHLD, sigchld_handler);
    signal(SIGTERM, sigterm_handler);

    //Configuring server
    if(argc != 3) {
        perror("Invalid arguments to start the server\n");
        return 1;
    }

    char path[1024];
    char line[1024];
    char* line2;
    char* token;
    int counter = 0;
    int counter2 = 0;
    ssize_t bytes_read = 0;

    strcpy(path, argv[1]);
    printf("Path %s\n", path);
    int fd = open(path, O_RDONLY);
    if(fd == -1) {
        perror("Error opening config file\n");
        return 1;
    }
    else {
        while ((bytes_read = readln(fd, line, 1024))) {
            line2 = strdup(line);
            while ((token = strtok_r(line2, " ", &line2))) {
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

    mkfifo("tmp/fifo_connection", 0644);
    //mkfifo("tmp/fifo_serverClient", 0644);

    //char **log = malloc(sizeof(char*) * 1);
    //int iLog = 0;
    while (!terminate) {
        char *buffer = malloc(sizeof(char) * MAX);
        char clientPipeName[50];

        //int fifo_serverClient = open("tmp/fifo_serverClient", O_RDWR);

        printf("Waiting for client\n");
        int fifo_connection = open("tmp/fifo_connection", O_RDONLY);
        if(read(fifo_connection, clientPipeName, 50) < 0){
            strcpy(buffer, "Erro");
        }
        close(fifo_connection);

        printf("CLIENT PIPE NAME - %s\n", clientPipeName);

        int fifo_clientServer = open(clientPipeName, O_RDONLY); 

        if (read(fifo_clientServer, buffer, MAX) < 0)
            strcpy(buffer, "Erro");
            
        if (strncmp(buffer, "ajuda", 5) == 0) { // Listar a maneira correta de utilizar o programa
            char comandos[] = "\nComandos diponiveis:\n\n"
                                "./aurras status\n"
                                "./aurras transform input-filename output-filename filter-id-1 filter-id-2...\n\n";
            int fifo_clientServer = open(clientPipeName, O_WRONLY); 
            write(fifo_clientServer, comandos, strlen(comandos));
            close(fifo_clientServer);
        }
        else if (strncmp(buffer, "transform", 9) == 0) { 
            int pipes[32][2];
            int currentPipe = 0;
            char message [64];
            char* args [1024];
            char* token;
            int num_filters = 0;
            char* filter_names = NULL;
            char* straux = NULL;
            char* straux2 = NULL;
            int counter = 0;
            int counter2 = 0;
            int fst = 0;
            int fst2 = 0;
            char* v = ", ";
            char* v2 = " ";
            char* str = strdup(clientPipeName);

            task_command[task_number] = strdup(buffer);
            task_status[task_number] = "EXECUTING";

            while ((token = strtok_r(str, "_", &str))){
                if(counter2 == 1){
                    clientPids[task_number] = token;
                }
                counter2++;
            }

            while ((token = strtok_r(buffer, " ", &buffer))) {
                if (counter == 3) {
                    if (!fst2) {
                        fst2 = 1;
                        filter_names = strdup(token);
                        num_filters++;
                        straux = strdup(getExecPath(token));
                        straux2 = strdup(getExec(token));
                    }   
                    else {
                        strcat(straux, getExec(token));
                        strcat(filter_names, token);
                        num_filters++;
                    }
                    strcat(straux, v);
                    strcat(straux2, v);
                    strcat(filter_names, v2);
                }
                else {
                    if (fst)
                        args[counter-1] = strdup(token);
                    fst = 1;
                    counter++;
                }
            }

            straux[strlen(straux)-2] = '\0';
            straux2[strlen(straux2)-2] = '\0';
            counter++;
            printf("Filter_names %s \n", filter_names);
            printf("STrAux %s \n", straux);
            printf("STrAux2 %s \n", straux2);
            for(int i = 0; args[i]; i++)
                printf("Args[%d] - %s\n", i, args[i]);

            int fst_time = 0;
            //fazer uma função checkfilter a correr num while para so continuar quando houver filtros disponiveis
            char* filter_names2 = strdup(filter_names);
            char* filter_names3 = strdup(filter_names);
            int fifo_clientServer = open(clientPipeName, O_WRONLY); 
            printf("FILTER NAMES - %s\n", filter_names);
            char* filter_names4 = strdup(filter_names);
            while (checkFiltersUsage(filter_names4)){
                if(!fst_time) {
                    fst_time = 1;
                    sprintf(message, "Pending task #%d\n", task_number+1);
                    write(fifo_clientServer, message, strlen(message));
                }
                strcpy(filter_names4, filter_names);
            }

            //Incrementar os filtros que vao ser usados
            incrementFilters(filter_names2);

            sprintf(message, "\nProcessing task #%d\n\n", task_number+1);
            write(fifo_clientServer, message, strlen(message));

            int stdin_faudio = open(args[0], O_RDONLY);
            int stdout_faudio = open(args[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);

            printf("Num filters - %d\n", num_filters);
            printf("FILTER NAMES - %s\n", filter_names3);

            int i = 0, pid;
            char *token2 = NULL;
            while ((token2 = strtok_r(filter_names3, " ", &filter_names3))) {
                printf("TOken2 %s\n", token2);
                printf("I - %d\n", i);
                printf("ExecPath - %s, Exec - %s\n", getExecPath(token2), getExec(token2));
                if (i == 0) {
                    pipe(pipes[currentPipe]);
                    if ((pid = fork()) == 0) {
                        close(pipes[currentPipe][0]);
                        dup2(stdin_faudio, STDIN_FILENO);
                        close(stdin_faudio);
                        if(num_filters == 1){
                            dup2(stdout_faudio, STDOUT_FILENO);
                            close(stdout_faudio);
                        }
                        else{
                            dup2(pipes[currentPipe][1], STDOUT_FILENO);
                            close(pipes[currentPipe][1]);
                        }
                        sleep(3);
                        execl(getExecPath(token2), getExec(token2), NULL);
                        exit(1);
                    }
                }
                else { //fazer um if a ver se e o ultimo filtro para redirecionar para o ficheiro
                    pipe(pipes[currentPipe]);
                    if ((pid = fork()) == 0) {
                        close(pipes[currentPipe][0]);
                        if (currentPipe != 0) {
                            dup2(pipes[currentPipe-1][0], STDIN_FILENO);
                            close(pipes[currentPipe-1][0]);
                        }
                        if(i+1==num_filters){
                            dup2(stdout_faudio, STDOUT_FILENO);
                            close(stdout_faudio);
                        }
                        else{
                            dup2(pipes[currentPipe][1], STDOUT_FILENO);
                            close(pipes[currentPipe][1]);
                        }
                        execl(getExecPath(token2), getExec(token2), NULL);
                        exit(1);
                    }
                }
                
                close(pipes[currentPipe][1]);
                pids[task_number][currentPipe] = pid;
                if (currentPipe != 0)
                    close(pipes[currentPipe-1][0]);
                currentPipe++;
                i++;
            }
            total_childs[task_number] = currentPipe;
            task_number++;
            close(fifo_clientServer);
        }

        else if (strncmp(buffer, "status", 6) == 0) {
            char message[1024];
            char endmessage[2048];
            char* token;
            char* str = strdup (clientPipeName);
            int counter = 0;
            int empty = 1;
            strcpy(endmessage, "Tasks Executing:\n");
            int fifo_clientServer = open(clientPipeName, O_WRONLY); 
            //write(fifo_clientServer, message, strlen(message));

            for (int i = 0; i < task_number; i++)
                if (strcmp(task_status[i], "EXECUTING") == 0) {
                    empty = 0;
                    sprintf(message, "task #%d: %s\n", i+1, task_command[i]);
                    strcat(endmessage, message);
                    //write(fifo_clientServer, message, strlen(message));
                }
            for (int i = 0; filter_array[i].name[0]; i++) {
                sprintf(message, "filter %s: %d/%d (running/max)\n", filter_array[i].name, filter_array[i].usage, filter_array[i].total);
                strcat(endmessage, message);
                //write(fifo_clientServer, message, strlen(message));
            }
            sprintf(message, "pid: %d\n", getpid());
            strcat(endmessage, message);
            //write(fifo_clientServer, message, strlen(message));
            strcat(endmessage, "\n");
            //write(fifo_clientServer, "\n", 1);
            if (empty) {
                sprintf(message, "There are no tasks executing\n");
                strcat(endmessage, message);
                //write(fifo_clientServer, message, strlen(message));
            }
            //Codigo para termianar
            //fifo_connection = open("tmp/fifo_connection", O_WRONLY);
            while ((token = strtok_r(str, "_", &str))){
                if(counter == 1){
                    strcat(endmessage, token);
                    //write(fifo_clientServer, token, strlen(token));
                }
                counter++;
            }
            strcat(endmessage, "\0");
            write(fifo_clientServer, endmessage, strlen(endmessage));
            //write(fifo_clientServer, '\0', 0);
            //close(fifo_connection);
            close(fifo_clientServer);
        }
    }
    return 0;
}