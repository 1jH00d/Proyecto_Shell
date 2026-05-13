// -----------------------------------------------------------------------------
// UNIX Shell Project
// function prototypes, macros and type declarations for job control
//
// Sistemas Operativos
// Dept. Arquitectura de Computadores - UMA
//
// -----------------------------------------------------------------------------

#ifndef _JOB_CONTROL_H
#define _JOB_CONTROL_H

#include "list.h"   // insert_item, remove_item, check_item, get_item_bypos

// -----------------------------------------------------------------------------
//      PUBLIC functions, enumerations, structures, types
// -----------------------------------------------------------------------------

// ----------- enumeration & strings -------------------------------------------
enum job_state { FOREGROUND, BACKGROUND, STOPPED };
static char* state_strings[] = { "Foreground", "Background", "Stopped" };

// ----------- TYPE FOR JOB ----------------------------------------------------
typedef struct
{
    pid_t pgid;     // group id = process lider id
    char *command;  // program name = argv[0]
    enum job_state state;
    // Add here new fields if required
    char **argv;
} job;


// Functions for job elements:
job * new_job(pid_t pid, const char *command, enum job_state state);
void  free_job(job * item);
job * get_job_bypid(list_head_t *list, pid_t pid); // get job from a list
job * get_job_bycmd(list_head_t *list, char *cmd); // get job from a list


// Funcions and macros for list of jobs
#define add_job(l,j)         insert_item(l,(void *)j)
#define del_job(l,j)         remove_item(l,(void *)j)
#define check_job(l,j)       find_item(l,(void *)j)
#define get_job_bypos(l,n)   (job*)get_item_bypos(l,n)
void print_job(job *, unsigned int);
void print_job_list(list_head_t *);


// -----------------------------------------------------------------------------
// to debug integer variable 'i', use:    debug(i,%d);
// to debug char* variable 'str', use:    debug(str,%s);
// it will print out:  current line number, function name and file name, and also variable name, value and type
#define debug(x,fmt) fprintf(stderr,"\"%s\":%u:%s(): --> %s= " #fmt " (%s)\n", __FILE__, __LINE__, __FUNCTION__, #x, x, #fmt)

#endif

