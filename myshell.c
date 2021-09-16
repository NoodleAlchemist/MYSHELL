#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#define MAX_STRLEN 2048
#define MAX_PARAMETERS 7
#define MAX_HISTORY 2048
#define MAX_ARGS 5


int flag = 0;

typedef struct node{
    char* string;
    int place;
    struct node* next;
}node;

void initShell(char **cd);

node* getCmd(char **, node*);

node* addHistory(char *buf, node*);

node* executeCmd(char*, char **, node*);

node* clearHistory(node*);

void printHistory(node* );

void initCmd(char**);

void saveHistory(node*);

node* fileHistory(node* sessionHistory);

void moveToDir(char* moveDir, char** cd);

char* getCmdToReplay(int num, node* sessionHistory);

void parseCmd(char **cmd, char *buf, char *delim, char slash);

void start(char *path, char **args, int background);

char** getArgs(char **cmd, char **str, char slash, int index);

char* setPath(char **cmd, char **buf, char *cd, char slash, int index, int flag);

void exterminate(int pid);

void main() {

    // init list of commands and current directory
    char **cmd = malloc(MAX_PARAMETERS * sizeof(char *));
    char *currentdir = malloc(PATH_MAX * sizeof(char));

    node* sessionHistory = NULL;

    // get history of entered commands from previous shell sessions
    sessionHistory = fileHistory(sessionHistory);

    // set cwd
    initShell(&currentdir);

    while(1){
        
        printf("%s #", currentdir);
        initCmd(cmd);
        // get next command and add to history
        sessionHistory = getCmd(cmd, sessionHistory);

        sessionHistory = executeCmd(currentdir, cmd, sessionHistory);
        
    }

}

void initCmd(char** cmd){
    for(int i = 0; i < MAX_PARAMETERS; i++){
        cmd[i] = NULL;
    }
}

// adds user entered command to history list
node* fileAddHistory(char* c, node* sessionHistory){
    node* tmp = sessionHistory;

    node* new = malloc(sizeof(node));
    new->next = NULL;
    new->string = malloc(MAX_STRLEN * sizeof(char));
    strcpy(new->string, c);

    if(tmp == NULL){
        sessionHistory = new;
        return sessionHistory;
    }

    while(tmp->next != NULL){
        tmp = tmp->next;
    }

    tmp->next = new;

    return sessionHistory;

}

//on startup will open txt file of previous commands and add them to history list
node* fileHistory(node* sessionHistory){
    char* c = malloc(MAX_STRLEN * sizeof(char));

    FILE *fp = fopen("history.txt", "a+");
    if(fp == NULL){
        printf("Could not open history.txt!\n");
        return sessionHistory;
    }

    while(fgets(c, MAX_STRLEN, fp)){
        c[strcspn(c, "\n")] = 0;
        sessionHistory = fileAddHistory(c, sessionHistory);
    }

    fclose(fp);

    return sessionHistory;
}

// gets cwd
void initShell(char **cd) {

    getcwd(*cd, PATH_MAX);
}

// sets each pointer in cmd to null
void initCmd(char** cmd){
    for(int i = 0; i < MAX_PARAMETERS; i++){
        cmd[i] = NULL;
    }
}

//get the user enter command
node* getCmd(char **cmd, node* sessionHistory) {

    char buf[MAX_STRLEN];
    char histCmd[MAX_STRLEN];

    fgets(buf, MAX_STRLEN, stdin);
    buf[strcspn(buf, "\n")] = 0;

    strcpy(histCmd, buf);
    // buf will just be first part of string
    parseCmd(cmd, buf, " ", 0);
    
    //dont add replay to session history
    if(strcmp(buf, "replay") != 0)
        sessionHistory = addHistory(histCmd, sessionHistory);


    return sessionHistory;

}

node* addHistory(char *buf, node *sessionHistory) {


    node* tmp = malloc(sizeof(node));
    tmp->string = malloc(MAX_STRLEN * sizeof(char));

    strcpy(tmp->string, buf);
    tmp->next = sessionHistory;
    sessionHistory = tmp;

    return sessionHistory;


}

node* executeCmd(char* cd, char **cmd, node* sessionHistory) {

    if (strcmp(cmd[0], "byebye") == 0){
        printf("BYE!\n");
        saveHistory(sessionHistory);
        exit(0);
    }

    else if (strcmp(cmd[0], "history") ==0){
        if(cmd[1] != NULL && strcmp(cmd[1], "-c") == 0){
            saveHistory(sessionHistory);
            sessionHistory = clearHistory(sessionHistory);
        }else
            printHistory(sessionHistory);
    }

    if (strcmp(cmd[0], "whereami") == 0){
        printf("%s\n", cd);
    }

    if(strcmp(cmd[0], "movetodir") == 0){
        moveToDir(cmd[1], &cd);
    }

    if(strcmp(cmd[0], "replay") == 0){

        int num = atoi(cmd[1]);

        char* cmdToReplay = getCmdToReplay(num, sessionHistory);

        if(cmdToReplay == NULL){
            printf("Command to replay does not exist!\n");
            return sessionHistory;
        }

        initCmd(cmd);
        sessionHistory = addHistory(cmdToReplay, sessionHistory);
        parseCmd(cmd, cmdToReplay, " ", 0);


        sessionHistory = executeCmd(cd, cmd, sessionHistory);
        return sessionHistory;
    }

    if(strcmp(cmd[0], "start") == 0 || strcmp(cmd[0], "background") == 0 || strcmp(cmd[0], "repeat") == 0)
    {
        char* buf = malloc((strlen(cmd[1]) + 1) * sizeof(char));
        char** str = malloc(sizeof (char*));
        char** args;
        char* path;
        char slash;
        int index = 1;
        int repeats = 1;
        int background = strcmp(cmd[0], "start") == 0 ? 0 : 1;

        if(strcmp(cmd[0], "repeat") == 0){
            index = 2;
            repeats = atoi(cmd[1]);
        }

        slash = cmd[index][0];
        path = setPath(cmd, &buf, cd, slash, index, 0);
        if(!path)
            return sessionHistory;
        // buf contains path, and string will be tokenized till program name
        parseCmd(str, buf, "/", slash);

        args = getArgs(cmd, str, slash, index);

        for(int i = 0; i < repeats; i++)
            start(path, args, background);

    }

    if(strcmp(cmd[0], "dalek") == 0){
        exterminate(atoi(cmd[1]));
    }

    if(strcmp(cmd[0], "dwelt") == 0){

        char slash;
        char* path;
        int index = 1;
        char* buf = malloc((strlen(cmd[1]) + 1) * sizeof(char));
        char** str = malloc(sizeof (char*));
        struct stat sb;


        slash = cmd[index][0];
        path = setPath(cmd, &buf, cd, slash, index, 0);
        if(!path)
            return sessionHistory;
        // buf contains path, and string will be tokenized till program name
        parseCmd(str, buf, "/", slash);
        
       
        if(stat(path, &sb) == 0 ){
            if(S_ISDIR(sb.st_mode))
                printf("Abode is\n");
            if(S_ISREG(sb.st_mode))
                printf("Dwelt indeed\n");
        }else
            printf("Dwelt not\n");
    }

    if(strcmp(cmd[0], "maik") == 0){
        char slash;
        char* path;
        int index = 1;
        char* buf = malloc((strlen(cmd[1]) + 1) * sizeof(char));
        char** str = malloc(sizeof (char*));
        struct stat sb;


        slash = cmd[index][0];
        path = setPath(cmd, &buf, cd, slash, index, 0);
        if(!path)
            return sessionHistory;
        // buf contains path, and string will be tokenized till program name
        parseCmd(str, buf, "/", slash);

        int fd = open(path, O_RDWR | O_CREAT | O_EXCL, INT_MAX);
        if(fd < 0){
            perror("Failed to creat file");
        }

        int sz = write(fd, "Draft", strlen("Draft"));
        close(fd);
    }

    if(strcmp(cmd[0], "coppy") == 0){
        
        char* source = malloc((strlen(cmd[1]) + 1) * sizeof(char));
        char* dest = malloc((strlen(cmd[2]) + 1) * sizeof(char));
        char buf;
        int n;

        if(cmd[1][0] == '/'){
            strcpy(source, cmd[1]);
        }
        else{
            char* buf = malloc((strlen(cmd[1]) + 1) * sizeof(char));
            source = setPath(cmd, &buf, cd, cmd[1][0], 1, 0);
        }
        if(cmd[2][0] == '/'){
            strcpy(dest, cmd[2]);
        }
        else{
            char* buf = malloc((strlen(cmd[2]) + 1) * sizeof(char));
            dest = setPath(cmd, &buf, cd, cmd[2][0], 2, 1);
        }

        int fdSource = open(source, O_RDONLY);
        if(fdSource < 0){
            perror("Failed to open file");
            return sessionHistory;
        }else{
            int fdDest = open(dest, O_RDWR | O_CREAT | O_EXCL, INT_MAX);
            if(fdDest < 0){
                perror("Failed to creat file");

            }else{
                while((n = read(fdSource, &buf, 1)) != 0){
                    write(fdDest, &buf, 1);
                }
                close(fdDest);
                close(fdSource);
            }
        }

        


    }

    return sessionHistory;

}


void exterminate(int pid) {
    if(kill(pid, SIGKILL) == 0)
        printf("exterminated pid: %d\n", pid);
    else
        printf("Unable to exterminate process: %d\n", pid);
}



char *setPath(char **cmd, char **buf, char *cd, char slash, int index, int flag) {

    char* path = malloc(MAX_STRLEN * sizeof(char));

    if(index == 2 && slash != '/' && !flag){
        printf("Repeat command must have a full path to the program to run!\n");
        return NULL;
    }

    strcpy(path, cd);
    strcat(path, "/");
    strcat(path, cmd[index]);

    if(slash == '/'){
        memset(path, 0, strlen(path));
        strcpy(path, cmd[index]);
        strcpy(*buf, cmd[index]);
    }

    return path;
}

char **getArgs(char **cmd, char **str, char slash, int index) {
    int i;
    int j;
    char** args = malloc(MAX_PARAMETERS * sizeof(char*));

    if(slash == '/'){

        args[0] = malloc((strlen(str[0]) + 1) * sizeof(char));
        strcpy(args[0], str[0]);
    }
    else{
        args[0] = malloc((strlen(cmd[1]) + 1) * sizeof(char));
        strcpy(args[0], cmd[1]);
    }


    for(i = index + 1; cmd[i] != NULL; i++){
        j = i - index;
        args[j] = malloc(MAX_STRLEN);
        strcpy(args[j], cmd[i]);
    }
    args[i - 1] = NULL;

    return args;
}

void parseCmd(char **cmd, char *buf, char* delim, char slash) {
    char *token = NULL;

    token = strtok(buf, delim);

    for (int (i) = 0; token != NULL; i++) {

        cmd[i] = malloc((strlen(token)+1) * sizeof(char));
        strcpy(cmd[i], token);
        token = strtok(NULL, delim);
        if(slash == '/' && token != NULL){
            free(cmd[0]);
            i--;
        }

    }

}

node* clearHistory(node* sessionHistory){

    if(sessionHistory->next != NULL){
        clearHistory(sessionHistory->next);
    }
    free(sessionHistory->string);
    free(sessionHistory);
    return NULL;
    
}

void saveHistory(node* sessionHistory){

    node* tmp = sessionHistory;

    if(flag){
        tmp = fileHistory(sessionHistory);
    }
    flag = 1;

    FILE *fp = fopen("history.txt", "w");
    //int j = 0;


    while (tmp != NULL){
        fprintf(fp, "%s\n", tmp->string);
        tmp = tmp->next;

    }
    fclose(fp);
}

void printHistory(node* sessionHistory){

    int j = 0;

    node* tmp = sessionHistory;

    while(tmp != NULL){
        printf("%d: %s\n", j, tmp->string);
        tmp = tmp->next;
        j++;
    }

}

void moveToDir(char* moveDir, char** cd){

    if(moveDir == NULL){
        printf("Please provide a path!\n");
        return;
    }
    DIR* dir = opendir(moveDir);
    if (dir) {
        strcpy(*cd, moveDir);
        closedir(dir);
    } else if (ENOENT == errno) {
        printf("Directory does not exist!\n");
    } else {
        printf("Could not find directory");
    }
}

char* getCmdToReplay(int num, node* sessionHistory){

    node* tmp = sessionHistory;
    char *retStr;
    int i = 0;

    if(tmp == NULL){
        printf("No history to replay!");
        return NULL;
    }

    while(tmp != NULL){
        if(i == num){
            retStr = malloc((strlen(tmp->string) + 1) * sizeof(char));
            strcpy(retStr, tmp->string);
            return retStr;
        }
        i++;
        tmp = tmp->next;
    }
    return NULL;
}

void start(char *path, char **args, int background) {

    pid_t pid = fork();

    if (pid == -1)
    {
        // error, failed to fork()
        printf("Failed to fork\n");
    }
    else if (pid > 0)
    {
        int status;
        if(!background){
            waitpid(pid, &status, 0);
            // sleep so child can finish printing PID
        }

        sleep(1);
        return;
    }
    else
    {
        // we are the child
        printf("%s pid: %d\n", args[0], getpid());
        execv(path, args);
        perror("Exec Failed\n");
        _exit(EXIT_FAILURE);   // exec never returns
    }
}


#pragma clang diagnostic pop
