#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <linux/limits.h>
#include <sys/resource.h>

// Global variables
char command[100] = {};
char *param[100] = {};
char cwd[PATH_MAX] = {};
char printedPath [1000] = {};
int status;
short int background = 0;
char variable[100] = {};
char exportedString[100] = {}; // for echo

// methods forward definition
void change_directory(char* param[]);
void read_input();
void evaluate_expression();
int parse_input();
void on_child_exit(int signum);
void shell();
int command_type_builtin(char* param[]);
void execute_shell_bultin(char* param[]);
void execute_command(char* param[]);
void setup_environment();
void write_to_log_file(char message[]);


int main(int argc, char* argv[]){
    signal(SIGCHLD,on_child_exit); 
    setup_environment();
    shell();
    return 0;
}

//Return type of the handler function should be void
void on_child_exit(int signum){
    // waitpid(signum, &status, 0);
    int wstat;
    pid_t	pid;
    while (1) {
        pid = wait3 (&wstat, WNOHANG,NULL);
        if (pid == 0)
            return;
        else if (pid == -1)
            return;
        else
            printf ("Return code: %d\n", wstat);
		}
    write_to_log_file("Child terminated\n");
    // }
   
}

void write_to_log_file(char message[]){
    FILE *fptr;
   fptr = fopen("/home/maria/Documents/OS Labs/Lab 1/file.txt","a");

   if(fptr == NULL)
   {
      printf("Error in file\n");   
      exit(1);             
   }
    fputs(message,fptr);
   fclose(fptr);
}

void setup_environment(){
    // /home/maria
    strcpy(cwd , "/home/maria");
    if(chdir(cwd) != 0){
        perror("Error in the path");
    };
    strcpy(printedPath, "~");
}

void reset_param(char *param[]){
    for (int i = 0; i < 100; i++)
        param[i] = NULL;
}

void shell(){
    do
    { 
        read_input();
        // clear the param
        reset_param(param);
        int count = parse_input();
        // Shell builtin commands
        if((strcmp(param[0],"cd") == 0 ) || (strcmp(param[0],"export") == 0) || (strcmp(param[0],"echo") == 0)){
            // printf("in cd , etc\n");
            execute_shell_bultin(param);
        }
        else{
            if(strcmp(param[count - 1], "&") == 0){
                // printf("In &\n");
                background = 1;
                param[count - 1] = NULL;
            }
            execute_command(param);
        }
    } while (strcmp(param[0],"exit") != 0);
}

int command_type_builtin(char* param[]){
    int value = 0;
    if (strcmp(param[0],"cd") == 0){
        value = 1;
    }
     else if(strcmp(param[0],"echo") == 0){
        value = 2;
    } 
    else if(strcmp(param[0],"export") == 0){
        value = 3;
    }
    return value;
}
void execute_shell_bultin(char* param[]){
    int value; 
    value = command_type_builtin(param);
    switch(value){
        case 1:
            change_directory(param);
            break;
        case 2:{
            // printf("In echo\n");
            char echoString[100] = {};
            char dummyString[100] = {};
            for(int i = 1; param[i] != NULL; i++){
                strcat(echoString, param[i]);
                strcat(echoString, " ");
            }
            if(echoString[0] == '\"'){
                int c = 0;
                for(int i = 1; i < strlen(echoString) - 2; i++){
                    dummyString[c++] = echoString[i];
                }
                puts(dummyString);
            }else{
                // printf("Heree in echo\n");
                puts(echoString);
            }
            break;
        } 
        case 3:{
            // printf("In export\n");
            char firstString[100] = {};
            char x[100] = {};
            short int flag = 0;
            int counter = 0, varCount = 0;
            for(int i = 1; param[i] != NULL; i++){
                strcat(firstString, param[i]);
                strcat(firstString, " ");
            }
            for(int i = 0; i < strlen(firstString); i++){
                if(firstString[i] == '='){
                    flag = 1;
                    continue;
                }
                if(flag == 1){
                    x[counter++] = firstString[i];
                }else{
                    variable[varCount++] = firstString[i];
                }
            }
            x[counter] = '\0';
            if(x[0] == '"'){
                // remove "" from the beginning and the end
                counter = 0;
                for(int i = 1; i < strlen(x) - 2; i++){
                    exportedString[counter++] = x[i];
                }
                exportedString[counter] = '\0';
            }else{
                // printf("heree\n");
                strcpy(exportedString, x);
            }
            if(setenv(variable,exportedString,1) != 0){
                perror("Error in sentev");
                exit(10);
            }
            break;
        }
            
        default:
            printf("In default\n");
            break;
    }
}

void change_directory(char* param[]){
    char previousPath[1000] = {};
    strcpy(previousPath, cwd);
    if(param[1] != NULL){
        strcpy(printedPath,param[1]);
        int size = strlen(printedPath); //Total size of string
        if(strcmp(printedPath, "~") == 0){
            strcpy(cwd, "/home/maria");
        } else if(strcmp(printedPath, "..") == 0){
            for(long i = strlen(cwd) - 1; i >= 0; i--){
                if(cwd[i] == '/'){
                    cwd[i] = '\0';
                    break;
                }
            }
            strcpy(printedPath, cwd);
        } else if(printedPath[0] == '/'){
            // absolute path
            strcpy(cwd, printedPath);
        } else if(printedPath[0] != '/') {
            // relative path
            strcat(cwd, "/");
            printedPath[size-1] = '\0';
            strcat(cwd,printedPath);
            strcpy(printedPath, cwd);
        }
    } else{
        // means go to the home 
        strcpy(printedPath, "~");
        strcpy(cwd, "/home/maria");
    }
    if(chdir(cwd) != 0){
        perror("Error in the path");
        chdir(previousPath);
        strcpy(printedPath, previousPath);
    };
    
}

void read_input(){
    printf("Terminal:%s$ ",printedPath);
    fgets(command, sizeof(command), stdin);  // read string
    evaluate_expression();
    command[strcspn(command, "\n")] = '\0';
}

void evaluate_expression(){
    char currentVarName[100] = {};
    short int flag = 0, changeFlag = 0;
    int countName = 0, counterCommand = 0;
    char newCommand[100] = {};
    int start = 0;
    
    for(int i = 0; i < strlen(command); i++){
        if(flag == 1 && (command[i] == ' ' || command[i] == '\n' || command[i] == '\"')){
            strcat(newCommand,getenv(currentVarName));
            counterCommand += strlen(getenv(currentVarName));
            flag = 0;
            
        }else if(flag == 1){
            currentVarName[countName++] = command[i];
        }
        if(command[i] == '$'){
            // printf("catching export\n");
            flag = 1;
            start = i;
            changeFlag = 1;
        }
        if(command[i] != '$' && flag == 0 && command[i] != '\"'){
            newCommand[counterCommand++] = command[i];
        }
    }
    newCommand[counterCommand++] = '\0';
    if(changeFlag == 1){
        strcpy(command, newCommand);
        // printf("Here\n");
    }
}

int parse_input(){
    char * token = strtok(command, " ");
    // i is a counter to counts the number of tokens
    int i = 0;
    // loop through the string to extract all tokens
    while(token != NULL) {
        param[i] = token;
        i++;
        token = strtok(NULL, " ");
    }
   // return the number of tokens 
    return i;
}

void execute_command(char* param[]){
    pid_t child_id = fork();
    if(child_id < 0){
        perror("Error");
    } 
    else if(child_id == 0){
        // child
        char *cmd = param[0];
        if(strcmp(cmd, "exit") == 0){;
            execvp(cmd, param);
        }else if(execvp(cmd, param) != 0){
            printf("Command '%s' not found\n", cmd);
            exit(EXIT_FAILURE);
        }
    }
    else {
        // parent
        if(background == 0){
            waitpid(child_id, &status, 0);
        }else{
            background = 0;
        }
    }
}