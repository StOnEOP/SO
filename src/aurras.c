#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>

#include "../includes/aurras.h"

int transformInput(char *arg) {
    if (strcasecmp(arg, "transform") == 0)
        return 1;
    else
        if (strcasecmp(arg, "status") == 0)
            return 2;
        else {
            if (strlen(arg) > 5) {
                char *configFileChecker = strchr(arg, '.');
                configFileChecker++;
                if (strcmp(configFileChecker, "conf") == 0)
                    return 3;
            }
        }

    return 0;
}

int main(int argc, char *argv[]) {
    //char toServer[250];
    
    if (argc < 2) {     // INFO
        printf("Info\n");
        //int fifo_clientServer = open("fifo_clientServer", O_WRONLY);
        //int fifo_serverClient = open("fifo_serverClient", O_RDONLY);

        //close(fifo_clientServer);
        //close(fifo_serverClient);
    }
    else {
        int caseS = 0;
        caseS = transformInput(argv[1]);

        switch(caseS) {
            case 1:     // Transform
                printf("Transform\n");

                break;
            case 2:     // Status
                printf("Status\n");

                break;
            case 3:     // Exec server
                printf("Exec server\n");

                break;
            default:
                printf("Default\n");

                break;
        }
        //int fifo_clientServer = open("fifo_clientServer", O_WRONLY);
        //int fifo_serverClient = open("fifo_serverClient", O_RDONLY);

        //close(fifo_clientServer);
        //close(fifo_serverClient);
    }

    return 0;
}