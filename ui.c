/**
 * @file
 *
 * user interface
 */

#include <stdio.h>
#include <readline/readline.h>
#include <locale.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stddef.h>

#include "history.h"
#include "logger.h"
#include "ui.h"
#include "shell.h"

static const char *good_str = "🤠";
static const char *bad_str  = "🤮";

static unsigned int current_num;

static int glob_status = 0;

static bool scripting = false;

static int readline_init(void);

static char *previous_hist = "";

static char *current_psearch = "";

static bool do_prefix;

static bool arrowing;

static bool up;

static bool down;

static int tab_loc;

static int built_loc;

static char **tab_completions;

static int tab_max;

static char *built_list[4] = {"exit", "jobs", "history", "cd"};


// initialize the user interface
void init_ui(void)
{

    LOGP("Initializing UI...\n");
    char *locale = setlocale(LC_ALL, "en_US.UTF-8");
    LOG("Setting locale: %s\n",
            (locale != NULL) ? locale : "could not set locale!");

    if(isatty(STDIN_FILENO) == false) {
        LOGP("Data piped in on stdin; entering scripting mode\n");
        scripting = true;
    }

    hist_init(100);

    tab_loc = 0;
    arrowing = false;
    do_prefix = false;
    current_num = 0;
    up = false;
    down = false;

    rl_startup_hook = readline_init;
}

void set_status(int input_status){
    glob_status = input_status;
}

void set_arrowing(void)
{
    arrowing = false;
}

//free the memory allocated for the user interface
void destroy_ui(void)
{
    for(int i = 0; i<tab_max; i++){
        free(tab_completions[i]);
    }
    free(tab_completions);
}

//display the location and status of the shell
char *prompt_line(void)
{
    const char *status = prompt_status() ? bad_str : good_str;

    char cmd_num[25];
    snprintf(cmd_num, 25, "%u", prompt_cmd_num());

    char *user = prompt_username();
    char *host = prompt_hostname();
    char *cwd = prompt_cwd();

    char *format_str = ">>-[%s]-[%s]-[%s@%s:%s]-> ";

    size_t prompt_sz
        = strlen(format_str)
        + strlen(status)
        + strlen(cmd_num)
        + strlen(user)
        + strlen(host)
        + strlen(cwd)
        + 1;

    char *prompt_str =  malloc(sizeof(char) * prompt_sz);

    snprintf(prompt_str, prompt_sz, format_str,
            status,
            cmd_num,
            user,
            host,
            cwd);

    return prompt_str;
}

char *prompt_username(void)
{
    return getlogin();
}

char *prompt_hostname(void)
{
    char *buf = malloc(253*sizeof(char));
    if(gethostname(buf, 253*sizeof(char))!=-1){
        return buf;
    } else {
        return "error";
    }
    
}

//get the current working directory
char *prompt_cwd(void)
{
    char *cwd = getcwd(NULL, 0);
    char *home = getenv("HOME");
    char *temp;

    if((temp = strstr(cwd, home))) {
        if(temp-cwd==0){
            static char buffer[4096];
            strncpy(buffer, cwd, temp-cwd);
            buffer[temp-cwd] = '\0';
            sprintf(buffer+(temp-cwd), "%s%s", "~", temp+strlen(home));
            return buffer;
        }
    }

    if(strcmp(cwd, home)==0){
        return "~";
    } else {
        return cwd;
    }
}

int prompt_status(void)
{
    return glob_status;
}

unsigned int prompt_cmd_num(void)
{
    return hist_last_cnum();
}

//read the inputed command
char *read_command(void)
{
    if(scripting == true) {
        char *line = NULL;
        size_t line_sz = 0;
        ssize_t read_sz = getline(&line, &line_sz, stdin);
        line[read_sz - 1] = '\0';
        if(read_sz == -1) {
            free(line);
            return NULL;
        }
        return line;
    } else {
        char *command;
        char *prompt = prompt_line();
        command = readline(prompt); //this prints the prompt
        free(prompt);
        return command;
    }
    
}

int readline_init(void)
{
    rl_bind_keyseq("\\e[A", key_up);
    rl_bind_keyseq("\\e[B", key_down);
    rl_variable_bind("show-all-if-ambiguous", "on");
    rl_variable_bind("colored-completion-prefix", "on");
    rl_attempted_completion_function = command_completion;
    return 0;
}

//navigate up with keyboard
int key_up(int count, int key)
{
    if(down){
        current_num-=2;
    }
    bool firsttimer = false;
    const char *new_line = "";
    if(strcmp(rl_line_buffer, "")==0) {
        do_prefix = false;
    }
    if(strcmp(rl_line_buffer, previous_hist)!=0){
        do_prefix = false;
    }
    if(!arrowing&&!do_prefix) {
        LOGP("Reset Num\n");
        current_num = hist_size();
    }
    if(hist_size()>0&&current_num<=hist_size()){
        if (!do_prefix) {
            if (strcmp(rl_line_buffer, "")==0||strcmp(rl_line_buffer, previous_hist)==0) {
                do_prefix = false;
                up = false;
                if(!arrowing){
                    arrowing = true;
                    current_num = hist_size();
                }
                if(current_num!=0){
                    current_num--;
                    LOG("Decrease to index: %u\n", current_num);
                    new_line = hist_search_cnum(index_to_cnum(current_num));
                } else {
                    new_line = hist_search_cnum(index_to_cnum(0));
                }
            } else {
                arrowing = false;
                do_prefix = true;
                LOGP("Reset Num\n");
                current_num = hist_size();
                current_psearch = strdup(rl_line_buffer);
                firsttimer = true;
                up = true;
                goto prefixup;
            }
        } else {
    prefixup:
            LOG("prefix_index was: %u\n", current_num);
            struct index_navigator nav = hist_search_prefix_index(current_psearch, current_num, true);
            if(nav.index!=-1) {
                current_num = nav.index;
                new_line = nav.result;
                LOG("prefix_index now: %u, %s\n", current_num, new_line);
            } else {
                if(firsttimer) {
                    new_line = rl_line_buffer;
                } else {
                    new_line = previous_hist;
                }
            }
        }
    }
    previous_hist = (char *) strdup(new_line);
    /* Modify the command entry text: */
    rl_replace_line(new_line, 1);

    /* Move the cursor to the end of the line: */
    rl_point = rl_end;

    return 0;
}

//navigate down with keyboard
int key_down(int count, int key)
{
    if(up) {
        current_num += 2;
    }
    bool firsttimer = false;
    const char *new_line = "";
    if(strcmp(rl_line_buffer, "")==0) {
        do_prefix = false;
    }
    if(strcmp(rl_line_buffer, previous_hist)!=0){
        do_prefix = false;
    }
    if(!arrowing&&!do_prefix) {
        LOGP("Reset Num\n");
        current_num = hist_size();
    }
    if(hist_size()>0&&current_num<hist_size()){
        if (!do_prefix) {
            if (strcmp(rl_line_buffer, "")==0||strcmp(rl_line_buffer, previous_hist)==0) {
                do_prefix = false;
                down = false;
                if(!arrowing){
                    arrowing = true;
                    current_num = hist_size();
                }
                if(current_num!=hist_size()-1){
                    current_num++;
                    LOG("Increase to index: %u\n", current_num);
                    new_line = hist_search_cnum(index_to_cnum(current_num));
                } else {
                    new_line = "";
                }
            } else {
                arrowing = false;
                do_prefix = true;
                LOGP("Reset Num\n");
                current_num = hist_size();
                current_psearch = strdup(rl_line_buffer);
                firsttimer = true;
                down = true;
                goto prefixup;
            }
        } else {
    prefixup:
            LOG("prefix_index was: %u\n", current_num);
            struct index_navigator nav = hist_search_prefix_index(current_psearch, current_num, false);
            if(nav.index!=-1) {
                current_num = nav.index;
                new_line = nav.result;
                LOG("prefix_index now: %u, %s\n", current_num, new_line);
            } else {
                if(firsttimer) {
                    new_line = "";
                } else {
                    new_line = "";
                }
            }
        }
    } 
    previous_hist = (char *) strdup(new_line);
    /* Modify the command entry text: */
    rl_replace_line(new_line, 1);

    /* Move the cursor to the end of the line: */
    rl_point = rl_end;

    return 0;
}

char **command_completion(const char *text, int start, int end)
{
    /* Tell readline that if we don't find a suitable completion, it should fall
     * back on its built-in filename completion. */
    rl_attempted_completion_over = 0;

    return rl_completion_matches(text, command_generator);
}

/**
 * This function is called repeatedly by the readline library to build a list of
 * possible completions. It returns one match per function call. Once there are
 * no more completions available, it returns NULL.
 */
char *command_generator(const char *text, int state)
{
    LOG("STATE: %d\n", state);

    if (state==0){
        free(tab_completions);
        tab_completions = calloc(10, sizeof(char *));
        tab_max = 10;
        tab_loc = 0;
        built_loc = 0;

        char *path = strdup(getenv("PATH")); //strdup

        LOG("text: %s\n", text);

        char **args = calloc(10, sizeof(char *));
        int arg_max = 10;
        int arg_size = 0;
        int tokens = 0;
        char *next_tok = path;
        char *curr_tok;
        while((curr_tok = next_token(&next_tok, ":")) != NULL) {
            arg_size++;
            if(!(arg_size<arg_max)) {
                arg_max*=2;
                char **temp_args = realloc(args, sizeof(char *)*arg_max);
                if(temp_args==NULL){
                    free(args);
                    exit(0);
                } else {
                    args = temp_args;
                }
            }
            args[tokens++] = strdup(curr_tok);
        }
        args[tokens] = NULL;

        for (int i = 0; i<arg_size; i++) {

            char *loc_path = args[i];
            LOG("path: %s\n", args[i]);
            DIR *directory_stream = opendir(strdup(args[i]));

            if (directory_stream == NULL) {
                if(i==arg_size-1){
                    goto builtins;
                } else {
                    continue;
                }
            }

            struct dirent* entry = NULL;

            while ((entry = readdir(directory_stream)) != NULL) {

                if(entry->d_name[0]!='.') {
                    struct stat stats;
                    char *combined_path = malloc(sizeof(char) * (strlen(loc_path) + strlen(entry->d_name) + 1));
                    strcpy(combined_path, loc_path);
                    strcat(combined_path, "/");
                    strcat(combined_path, entry->d_name);
                    if(stat(combined_path, &stats)==-1) {
                        free(combined_path);
                        perror("stat");
                    } else {
                        free(combined_path);
                        if(stats.st_mode==33261){
                            if(strncmp(entry->d_name, text, strlen(text))==0){
                                if (tab_loc<tab_max) {
                                    LOG("Addding: %s\n", entry->d_name);
                                    tab_completions[tab_loc] = strdup(entry->d_name);
                                    tab_loc++;
                                } else {
                                    tab_max*=2;
                                    char **tab_completions_temp = realloc(tab_completions, sizeof(char *)*tab_max);
                                    if(tab_completions_temp==NULL){
                                        free(tab_completions);
                                        free(path);
                                        exit(0);
                                    } else {
                                        tab_completions = tab_completions_temp;
                                    }
                                    LOG("Addding: %s\n", entry->d_name);
                                    tab_completions[tab_loc] = strdup(entry->d_name);
                                }
                            }
                        }
                    }
                }
            }
        }


        tab_loc = 0;
        built_loc = 0;
        if (tab_completions[tab_loc]==NULL){
            free(path);
            goto builtins;
        }
        free(path);
        return tab_completions[tab_loc];

        
    } else {
        if (tab_loc+1<tab_max) {
            tab_loc++;
            if (tab_completions[tab_loc]!=NULL){
                LOG("Returning: %s\n", tab_completions[tab_loc]);
                return tab_completions[tab_loc];
            } else {
                goto builtins;
            }
        } else {
            goto builtins; 
        }
    }

builtins:
    for(int i = built_loc; i<4; i++){
        if(text[0]=='\0'){
            built_loc++;
            return built_list[i];
        } else {
            if(strlen(built_list[i])>strlen(text)&&strncmp(text, built_list[i], strlen(text))==0){
                LOG("Returning: %s\n", built_list[i]);
                built_loc++;
                return built_list[i];
            }
        }
        built_loc++;
    }

    return NULL;
}