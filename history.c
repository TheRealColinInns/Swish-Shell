/**
 * @file
 *
 * history
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

static unsigned int command_num;

static unsigned int list_limit;

static unsigned int list_index;

static bool maxed;

struct history *hist_list;

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

void hist_add(char *cmd)
{
    if(strcmp(cmd, "")!=0){
        if(!maxed){
            hist_list[list_index].cmd_num = command_num;
            hist_list[list_index].command = strdup(cmd);
            //LOG("added %s to index %d\n", hist_list[list_index].command, hist_list[list_index].cmd_num);
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

void hist_print(void)
{
    for(int i = 0; i<list_limit; i++){
    	if(strcmp(hist_list[i].command, "ENDOFLIST")!=0){
        	printf("%u %s\n", hist_list[i].cmd_num, hist_list[i].command);
    	}
    }
}

const char *hist_search_prefix(char *prefix)
{
    for(int i = list_limit-1; i>=0; i--){
    	if(hist_list[i].command!=NULL&&strncmp(hist_list[i].command, prefix, strlen(prefix))==0){
    		return strdup(hist_list[i].command);
    	}

    }
    return NULL;
}

const char *hist_search_cnum(int command_number)
{
    for(int i = 0; i<list_limit; i++){
    	//LOG("%d::%d\n", command_number, hist_list[i].cmd_num);
    	if(hist_list[i].cmd_num==command_number){
    		return strdup(hist_list[i].command);
    	}
    }
    return NULL;
}

struct index_navigator hist_search_prefix_index(char *prefix, int start_index, bool up) {
    /*
    if(start_index>=list_limit) {
        start_index = list_limit-1;
    }
    */
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
unsigned int hist_last_cnum(void)
{
    return command_num-1;
}
unsigned int hist_bottom_cnum(void)
{
	return hist_list[0].cmd_num;
}
unsigned int hist_get_limit(void)
{
	return list_limit-1;
}
unsigned int index_to_cnum(int index)
{
	return hist_list[index].cmd_num;
}
