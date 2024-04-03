/**
 * @file
 *
 * Contains shell history data structures and retrieval functions.
 */
#include <stddef.h>
#include <stdbool.h>
#ifndef _HISTORY_H_
#define _HISTORY_H_

//struct to hold history data
struct history
{
	unsigned int cmd_num;
	char *command;
};

//struct to be return both a result and the index
struct index_navigator
{
	unsigned int index;
	const char *result;
};

unsigned int hist_get_limit(void);
void hist_init(unsigned int);
void hist_destroy(void);
void hist_add(char *);
void hist_print(void);
const char *hist_search_prefix(char *);
struct index_navigator hist_search_prefix_index(char *, int, bool);
const char *hist_search_cnum(int);
unsigned int hist_last_cnum(void);
unsigned int hist_bottom_cnum(void);
unsigned int hist_size(void);
unsigned int index_to_cnum(int);

#endif