/**
 * @file
 *
 * shell (main)
 */

#include <fcntl.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>

#include "history.h"
#include "logger.h"
#include "ui.h"
#include "shell.h"

static bool piping = false;

static size_t job_size = 0;

static struct process jobs[10];

static bool running = true;

void sigint_handler(int signo)
{
    LOGP("Attempted to Exit\n");
}
void sigchld_handler(int signo)
{
    if (running) {
        int status_local;
        pid_t pid = waitpid(-1,&status_local,WNOHANG);
        
        if(pid>0){
            //LOG("PID: %d\n", pid);
            for(int i = 0; i<job_size; i++){
                if(jobs[i].background_pid==pid) {
                    LOG("Match Found at %d Replacing... %s\n", jobs[i].background_pid, jobs[i].command);
                    jobs[i].background_pid = 0;
                    free(jobs[i].command);
                    jobs[i].command = NULL;
                    for(int j = i; j<job_size; j++) {
                        jobs[j] = jobs[j+1];
                    }
                    job_size--;
                }
            }
        }
    }
}
void free_jobs(void)
{
    running = false;
    for(int i = 0; i<=job_size; i++){
        LOG("Freeing: %s\n", jobs[i].command);
        free(jobs[i].command);
    }
}
char *next_token(char **str_ptr, const char *delim)
{
    if (*str_ptr == NULL) {
        return NULL;
    }

    size_t tok_start = strspn(*str_ptr, delim);
    size_t tok_end = strcspn(*str_ptr + tok_start, delim);

    if (tok_end  == 0) {
        *str_ptr = NULL;
        return NULL;
    }

    char *current_ptr = *str_ptr + tok_start;

    *str_ptr += tok_start + tok_end;

    if (**str_ptr == '\0') {
        *str_ptr = NULL;
    } else {
        **str_ptr = '\0';
        (*str_ptr)++;
    }

    return current_ptr;
}

size_t get_size(size_t size, char **args)
{
    size_t char_size = strlen(args[0]);
    for(int i = 1; i<size; i++){
        char_size++;
        char_size += strlen(args[i]);
    }
    return char_size;

}

int main(void)
{
    init_ui();
    
    piping = false;

    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);

    char *command;
    while (true) {
        command = read_command();
        if (command == NULL) {
            break;
        }

        //LOG("Original command: %s\n", command);

        //char *args[100]; //BAD
        //LOGP("Attempting Calloc\n");
        char **args = (char **) calloc(11, sizeof(char *));
        //LOGP("Calloc Success\n");
        int arg_max=10;

        int tokens = 0;
        int arg_size = 0;
        char *next_tok = command;
        char *curr_tok;
        while((curr_tok = next_token(&next_tok, " \t\r\n")) != NULL) {
            arg_size++;
            if(!(arg_size<arg_max)) {
                //LOGP("Attempting Realloc\n");
                arg_max*=2;
                char **temp_args = realloc(args, sizeof(char *)*arg_max);
                if (temp_args==NULL) {
                    free(args);
                    exit(0);
                } else {
                    args = temp_args;
                }
                //LOGP("Realloc Success\n");
            }
            if(curr_tok[0]=='|'||curr_tok[0]=='>'||curr_tok[0]=='<'){
                piping=true;
            }
            args[tokens++] = curr_tok;
        }
        args[tokens] = NULL;

        if(args[0] == (char *) 0) {
            goto cleanup;
        }

        if(piping){
            if(construct_pipeline(args, arg_size)==0) {
                //LOGP("Finished Pipeline\n");
            } else {
                LOGP("Pipeline Failure\n");
            }
            piping = false;
            goto cleanup;
        } 

        //check for builtins
        int builtin = builtins(args, arg_size, false);

        if(builtin!=0){
            if(builtin==-1){
                LOGP("Exiting\n");
                free(command);
                hist_destroy();
                free_jobs();
                exit(0);
            } else if(builtin==1){
                goto cleanup;
            }
        }


        if(args[0] == (char *) 0) {
            goto cleanup;
        } else {

            //need to find new size of array if there was comments
            for(int i = 0; i<arg_size; i++){
                if(args[i]==NULL || strcmp(args[i], "")==0){
                    arg_size = i;
                    break;
                } 
            }

            args[arg_size] = NULL;

            //convert the new array to something we can pass to history
            char hist_buf[get_size(arg_size, args)+1];
            strcpy(hist_buf, args[0]);

            for(int i = 1; i<arg_size; i++){
                strcat(hist_buf, " ");
                strcat(hist_buf, args[i]);
            }

            //add to history
            hist_add(hist_buf);

            
            if(execute(args)==-1){
                goto cleanup;
            }
            
        }


cleanup:
        /* We are done with command; free it */
        //LOGP("Free Command\n");
        free(command);
        free(args);
        arg_size = 0;
    }
    free_jobs();
    hist_destroy();
    return 0;
}

int seperate_args(char **args, char *original, size_t size){
    int tokens = 0;
    int counter = 0;
    char *next_tok = original;
    char *curr_tok;
    while((curr_tok = next_token(&next_tok, " \t\r\n")) != NULL) {
        counter++;
        if(curr_tok[0]=='|'||curr_tok[0]=='>'||curr_tok[0]=='<'){
            piping=true;
        }
        args[tokens++] = curr_tok;
    }
    args[tokens] = NULL;
    return counter;
}

int execute_pipeline(struct command_line *cmds)
{
    int counter = 0;
    while(cmds[counter].stdout_pipe){
        /* Creates a pipe. */
        int fd[2];
        if (pipe(fd) == -1) {
            perror("pipe");
            return -1;
        }
        
        pid_t pid = fork();
        if (pid == 0) {
            dup2(fd[1], STDOUT_FILENO);
            close(fd[0]);
            execvp(cmds[counter].tokens[0], cmds[counter].tokens);
        } else {
            dup2(fd[0], STDIN_FILENO);
            close(fd[1]);
            counter++;
        }
    }

    if (cmds[counter].stdout_file!=NULL) {
        if (cmds[counter].append) {
            int fd = open(cmds[counter].stdout_file, O_WRONLY | O_APPEND, 0666);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        } else {
            int fd = open(cmds[counter].stdout_file, O_WRONLY | O_TRUNC | O_CREAT, 0666);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
    }

    if (cmds[counter].stdin_file != NULL) {
        int fd = open(cmds[counter].stdin_file, O_RDONLY);
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    execvp(cmds[counter].tokens[0], cmds[counter].tokens);

    return 0;
    
}

int construct_pipeline(char **args, size_t size)
{
    
    struct command_line cmds[400] = { 0 };
    if(size>0){
        int cmds_counter = 0;
        cmds[0].tokens = &args[0];
        for(int i = 0; i<size; i++){
            //LOG("cmds: %d\n", cmds_counter);
            if(args[i][0]=='|') {
                args[i] = NULL;
                cmds[cmds_counter].stdout_pipe = true;
                cmds_counter++;
                cmds[cmds_counter].tokens = &args[i + 1];
            } else if(args[i][0]=='>'){
                if (args[i][1] == '>') {
                    cmds[cmds_counter].append = true;
                }
                args[i] = NULL; // remove > from args
                cmds[cmds_counter].stdout_file = args[i + 1];

            } else if(args[i][0] == '<'){
                args[i] = NULL; // remove < from args
                cmds[cmds_counter].stdin_file = args[i + 1];
            }
        }

        pid_t child = fork();
        if(child == -1) {
            perror("fork");
            return -1;
        } else if (child == 0) {
            if(execute_pipeline(cmds) != 0) {
                close(fileno(stdin));
                perror("execute_pipeline");
                exit(1);
            } 
        } else {
            int status_local;
            wait(&status_local);
            set_status(status_local);
        }
        return 0;
        
    }
    return -1;
}

int execute(char **args)
{
    pid_t child = fork();
    if(child == -1) {
        perror("fork");
        return -1;
    } else if (child == 0) {
        if(execvp(args[0], args) != 0) {
            close(fileno(stdin));
            perror("execvp");
            exit(1);
        } 
    } else {
        int status_local;
        waitpid(child, &status_local, 0);
        set_status(status_local);
    }
    return 0;
}

int background_execute(char **args, size_t arg_size)
{
    //LOGP("Background+\n");
    pid_t child = fork();
    if(child == -1) {
        perror("fork");
        return -1;
    } else if (child == 0) {
        if(execvp(args[0], args) != 0) {
            close(fileno(stdin));
            perror("execvp");
            exit(1);
        }
    } else {

        LOG("Saving PID: %d\n", child);
        char hist_buf[get_size(arg_size, args)+1];
        strcpy(hist_buf, args[0]);

        for(int i = 1; i<arg_size; i++){
            strcat(hist_buf, " ");
            strcat(hist_buf, args[i]);
        }

        LOG("Adding: %s\n", hist_buf);
        
        if (job_size<10) {
            struct process job_struct = { .command=strdup(hist_buf), .background_pid=child};
            jobs[job_size] = job_struct;
            if (job_size!=9) {
                job_size++;
            }
        }
    
    }
    //LOGP("Background-\n");

    return 0;
}

int builtins(char **args, size_t arg_size, bool bang) 
{
    bool comment = false;


    
    for(int i = 0; i<arg_size; i++){
        for(int j = 0; j < strlen(args[i]); j++){
            if(args[i][j]=='#'){
                if(i==0){
                    return 1;
                }
                comment = true;
            }
            if(comment){
                //LOG("Replacing %s with null byte\n", args[i]);
                args[i][j] = '\0';
            }
        }
    }
    
    char *cmd = args[0];
    if(strcmp(cmd, "exit") == 0){
        return -1;
    } else if(strcmp(cmd, "cd") == 0){
        if(arg_size==1){
            chdir(getenv("HOME"));
        } else if(arg_size==2) {
            if(strcmp(args[1], "..")==0){
                chdir("..");
            } else {
                chdir(args[1]);
            }
        } else {
            LOGP("Invalid CD command\n");
        }
        return 1;
    } else if(strcmp(cmd, "history")==0){
        if(!bang) {
            hist_add("history");
        }
        hist_print();
        fflush(stdout);
        return 1;
    } else if(cmd[0]=='!'){
        //LOG("CMD: %s\n", cmd);
        if(strlen(cmd)>2&&isdigit(cmd[1])){
            //get rid of that pesky !
            for(int i = 0; i<strlen(cmd); i++){
                cmd[i] = cmd[i+1];
            }
            //convert the rest
            long num = strtol(cmd, NULL, 10);
            const char *result = hist_search_cnum(num);
            //LOG("Result: %s\n", result);
            //execute the command
            if(result!=NULL){
                hist_add((char *) result);
                char *args_loc[100];
                seperate_args(args_loc, (char *) result, 0);
                //check for builtins
                int builtin = builtins(args_loc, arg_size, true);

                if(builtin!=0){
                    if(builtin==-1){
                        hist_destroy();
                        exit(0);
                    } else if(builtin==1){
                        return 1;
                    }
                }
                execute(args_loc);
            }
            free((char *) result);
            return 1;
        } else if(strlen(cmd)>=2&&isalpha(cmd[1])){
            //get rid of !
            for(int i = 0; i<strlen(cmd); i++){
                cmd[i] = cmd[i+1];
            }
            const char *result = hist_search_prefix(cmd);
            //LOG("Result: %s\n", result);
            //execute the command
            if(result!=NULL){
                hist_add((char *) result);
                char *args_loc[100];
                seperate_args(args_loc, (char *) result, 0);
                //check for builtins
                int builtin = builtins(args_loc, arg_size, true);

                if(builtin!=0){
                    if(builtin==-1){
                        free_jobs();
                        hist_destroy();
                        free((char *) result);
                        exit(0);
                    } else if(builtin==1){
                        return 1;
                    }
                }
                
                execute(args_loc);
            }
            free((char *) result);
            return 1;
        } else if(strcmp(cmd, "!!")==0){
            const char *result = hist_search_cnum(hist_last_cnum());
            //LOG("Result: %s\n", result);
            //execute the command
            if(result!=NULL){
                hist_add((char *) result);
                char *args_loc[100];
                seperate_args(args_loc, (char *) result, 0);
                //check for builtins
                int builtin = builtins(args_loc, arg_size, true);

                if(builtin!=0){
                    if(builtin==-1){
                        exit(0);
                    } else if(builtin==1){
                        return 1;
                    }
                }

                execute(args_loc);
            }
            free((char *) result);
            return 1;
        } else {
            LOGP("error\n");
            return 0;
        }
    } else if(strcmp(args[arg_size-1], "&")==0){
        //LOG("Remove &: %s\n", args[arg_size-1]);
        args[arg_size-1] = NULL;
        
        background_execute(args, arg_size-1);

        return 1;
    } else if (strcmp(cmd, "jobs")==0) {
        for (int i = 0; i<job_size; i++) {
            if (jobs[i].command!=NULL){
                printf("%s\n", jobs[i].command);
            }
        }
        return 1;
    }
    else {
        return 0;
    }
}