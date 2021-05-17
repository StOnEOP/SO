#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#include "../includes/aurrasd.h"
#define MAX 2048

struct filter{
    char name[50]; 
    char exec_path[100];
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

char* getExec (char* token){
    for(int i= 0; filter_array[i].name[0]; i++){
        if(strcmp(filter_array[i].name, token) == 0){
            return filter_array[i].exec_path;
        }
    }
    return "ERRO";
}

int checkFiltersUsage (char* filters){
    char* token;
    while((token = strtok_r(filters, " ", &filters))){
        for(int i = 0; filter_array[i].name[0]; i++){
            if(strcmp(filter_array[i].name, token) == 0){
                if(filter_array[i].usage == filter_array[i].total){
                    printf("Usage %d, Total %d\n", filter_array[i].usage, filter_array[i].total);
                    return 1;
                }
            }
        }
    }
    return 0;
}

void incrementFilters (char* filters){
    char* token;
    while((token = strtok_r(filters, " ", &filters))){
        for(int i = 0; filter_array[i].name[0]; i++){
            if(strcmp(filter_array[i].name, token) == 0){
                filter_array[i].usage++;
            }
        }
    }
}

void sigchld_handler(int sig){
    int status;
    pid_t pid;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        for(int i = 0; i <= task_number; i++){
            if(WEXITSTATUS(status) == 1){
                kill(pid, SIGTERM);
                task_status[i] = "ERROR";
                break;
            }
            else if(task_status[i] == "EXECUTING"){
                kill(pid, SIGTERM);
                task_status[i] = "FINISHED";
            }
        }
    }
}

void sigterm_handler(int sig){
    terminate = 1;
    int tasks_terminated = 0;
    int counter = 0;

    while(!tasks_terminated){
        counter = 0;
        for(int i = 0; i<= task_number; i++){
            if(task_status[i] == "EXECUTING")
                counter++;
        }
        if(!counter)
            tasks_terminated = 1;
        sleep(3);
    }

}

int main(int argc, char *argv[]) {
    //Configuring server
    if(argc != 3){
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

    strcpy(path, argv[2]);
    strcat(path, bar);
    strcat(path, argv[1]);
    printf("Path %s\n", path);
    int fd = open(path, O_RDONLY);
    if(fd == -1){
        perror("Error opening config file\n");
        return 1;
    }
    else{
        while((bytes_read = readln(fd, line, 1024))){
            //printf("%s\n", line);
            line2 = strdup(line);
            while((token = strtok_r(line2, " ", &line2))){
                //printf("Token %s\n", token);
                if(counter2 == 0)
                    strcpy(filter_array[counter].name, token);
                if(counter2 == 1)
                    strcpy(filter_array[counter].exec_path, token);
                if(counter2 == 2)
                    filter_array[counter].total = atoi(token);
                counter2++;
            }
            filter_array[counter].usage = 0;
            counter2 = 0;
            counter++;
        }
    }
    for(int i = 0; i<100 && filter_array[i].name[0]; i++){
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
            
        if(strncmp(buffer, "ajuda", 5) == 0) { //Listar a maneira correta de utilizar o programa
            char comandos[] = "\nComandos diponiveis:\n\n"
                                "./aurras status\n"
                                "./aurras transform input-filename output-filename filter-id-1 filter-id-2...\n\n";
            write(fifo_serverClient, comandos, strlen(comandos));
        }

        else if(strncmp(buffer, "transform", 9) == 0){ // ffmpeg -i input.mp3 -filter "filtro_1, filtro_2" output.mp3
            char message [64];
            char* args [1024];
            char* token;
            char* filter_names = NULL;
            char* straux = NULL;
            int counter = 2;
            int fst = 0;
            int fst2 = 0;
            char* v = ", ";
            char* v2 = " ";

            args[0] = "ffmpeg";
            args[1] = "-i";

            task_command[task_number] = strdup(buffer);
            task_status[task_number] = "EXECUTING";

            while((token = strtok_r(buffer, " ", &buffer))){
                if(counter == 5){
                    if(!fst2){
                        fst2 = 1;
                        filter_names = strdup(token);
                        straux = strdup(getExec(token));
                    }   
                    else{
                        strcat(straux, getExec(token));
                        strcat(filter_names, token);
                    }
                    strcat(straux, v);
                    strcat(filter_names, v2);
                }
                else{
                    if(fst)
                        args[counter-1] = strdup(token);  // ffmpeg -i input.mp3 output.mp3 filtro_1 filtro_2
                    fst = 1;
                    counter++;
                }
            }

            straux[strlen(straux)-2] = '\0';
            args[counter-1] = straux;
            counter++;
            token = args[3];
            args[3] = "-filter";
            args[counter-1] = token;    // ffmpeg -i input.mp3 -filter "filtro_1 filtro_2" output.mp3
            for(int i = 0; args[i]; i++){
                printf("Args[%d] - %s\n", i, args[i]);
            }

            int fst_time = 0;
            //fazer uma função checkfilter a correr num while para so continuar quando houver filtros disponiveis
            char* filter_names2 = strdup(filter_names);
            while(checkFiltersUsage(filter_names)){
                if(!fst_time){
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

            int pid;
            if((pid = fork()) == 0){
                execvp(args[0], args);
                exit(1);
            }

            task_number++;



        }

        else if(strncmp(buffer, "status", 6) == 0){
            char message[1024];
            int empty = 1;
            strcpy(message, "\nTasks Executing:\n");
            write(fifo_serverClient, message, strlen(message));

            for(int i = 0; i < task_number; i++){
                if(strcmp(task_status[i], "EXECUTING") == 0){
                    empty = 0;
                    sprintf(message, "task #%d: %s\n", i+1, task_command[i]);
                    write(fifo_serverClient, message, strlen(message));
                }
            }
            for(int i = 0; filter_array[i].name[0]; i++){
                sprintf(message, "filter %s: %d/%d (running/max)\n", filter_array[i].name, filter_array[i].usage, filter_array[i].total);
                write(fifo_serverClient, message, strlen(message));
            }
            sprintf(message, "pid: %d\n", getpid());
            write(fifo_serverClient, message, strlen(message));
            write(fifo_serverClient, "\n", 1);
            if(empty){
                sprintf(message, "There are no tasks executing\n");
                write(fifo_serverClient, message, strlen(message));
            }
        }
    }
    return 0;
}

