/**
 * @file
 *
 * history
 * 
 * tracks user's command history
 */

#include <stddef.h>
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

#include "history.h"
#include "logger.h"

static unsigned int command_num; //total number of commands

static unsigned int list_limit; //the max amount of commands to store in history

static unsigned int list_index; //the current index

static bool maxed; //boolean true if limit has been reached

struct history *hist_list; //our history of commands

//initialize history at start of program and gets the memory neccesary
void hist_init(unsigned int limit)
{
    LOG("Hist with limit %u created\n", limit);
    list_limit = limit;
    command_num = 1;
    list_index = 0;
    maxed = false;
    hist_list = calloc(limit+1, sizeof(struct history));
    for(int i = 0; i<=limit; i++) {
        hist_list[i].command = "ENDOFLIST";
    }
}

//release all the memory related to the history
void hist_destroy(void)
{
	LOGP("hist_destroy\n");
	for(int i = 0; i<list_limit; i++){
    	if(strcmp(hist_list[i].command, "ENDOFLIST")!=0){
        	free(hist_list[i].command);
    	}
    }
    free(hist_list);
}

//add a command to the history
void hist_add(char *cmd)
{
    if(strcmp(cmd, "")!=0){
        if(!maxed){
            hist_list[list_index].cmd_num = command_num;
            hist_list[list_index].command = strdup(cmd);
            list_index++;
        } else {
            for(int i=1; i<list_limit; i++){
                hist_list[i-1] = hist_list[i];
            }
            hist_list[list_limit-1].command = strdup(cmd);
            hist_list[list_limit-1].cmd_num = command_num;
        }
        command_num++;
        if(list_index>=list_limit){
            maxed = true;
        }
    }
    
}

//print the current history
void hist_print(void)
{
    for(int i = 0; i<list_limit; i++){
    	if(strcmp(hist_list[i].command, "ENDOFLIST")!=0){
        	printf("%u %s\n", hist_list[i].cmd_num, hist_list[i].command);
    	}
    }
}

//search the history list by a prefix (to be used by autocomplete)
const char *hist_search_prefix(char *prefix)
{
    for(int i = list_limit-1; i>=0; i--){
    	if(hist_list[i].command!=NULL&&strncmp(hist_list[i].command, prefix, strlen(prefix))==0){
    		return strdup(hist_list[i].command);
    	}
    }
    return NULL;
}

//search the history by the command number
const char *hist_search_cnum(int command_number)
{
    for(int i = 0; i<list_limit; i++){
    	if(hist_list[i].cmd_num==command_number){
    		return strdup(hist_list[i].command);
    	}
    }
    return NULL;
}

//search the history for a command starting with a prefix starting from a certain index
struct index_navigator hist_search_prefix_index(char *prefix, int start_index, bool up) {
	if (!up) {
		for(int i = start_index; i<list_limit; i++){
    		if(hist_list[i].command!=NULL&&strncmp(hist_list[i].command, prefix, strlen(prefix))==0){
                if (i<list_limit-1) {
    			    struct index_navigator return_struct = { .result=strdup(hist_list[i].command), .index=i+1};
                    return return_struct;
                } else {
                    struct index_navigator return_struct = { .result=strdup(hist_list[i].command), .index=i};
                    return return_struct;
                }
    		}
    	}
	} else {
		for(int i = start_index; i>=0; i--){
    		if(hist_list[i].command!=NULL&&strncmp(hist_list[i].command, prefix, strlen(prefix))==0){
                if (i>0) {
    			    struct index_navigator return_struct = { .result=strdup(hist_list[i].command), .index=i-1};
                    return return_struct;
                } else {
                    struct index_navigator return_struct = { .result=strdup(hist_list[i].command), .index=i};
                    return return_struct;
                }
    		}
    	}
	}
    LOGP("Index Navigator Unable to Find\n");
    struct index_navigator return_struct = { .result="bad", .index=-1 };
    return return_struct;
    
}

//gives the current size of the history
unsigned int hist_size(void)
{
	for(unsigned int i = 0; i<=list_limit; i++){
		if(strcmp(hist_list[i].command, "ENDOFLIST")==0){
			return i;
		} 
	}
    LOG("hist_size: %d\n", 0);
	return 0;
}

//return last command number
unsigned int hist_last_cnum(void)
{
    return command_num-1;
}

//return the last command number still in the list
unsigned int hist_bottom_cnum(void)
{
	return hist_list[0].cmd_num;
}

//return the history limit
unsigned int hist_get_limit(void)
{
	return list_limit-1;
}

//convert an index to a command number
unsigned int index_to_cnum(int index)
{
	return hist_list[index].cmd_num;
}
