// -----------------------------------------------------------------------------
// UNIX Shell Project
// job control
//
// Sistemas Operativos
// Grados I. Informatica, Computadores & Software
// Dept. Arquitectura de Computadores - UMA
//
// Adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.
// -----------------------------------------------------------------------------

#include <stdio.h>          // printf, stderr
#include <stdlib.h>         // malloc, free
#include <string.h>         // strcmp
#include <signal.h>         // signal, sigprocmask,...
#include "list.h"           // traverse_list, get_item_byfunc
#include "job_control.h"    // job type, check prototypes match with implemented
#include "parse_line.h"     // free_args

// -----------------------------------------------------------------------------
//  FUNCTIONS for JOBS management
// -----------------------------------------------------------------------------
// allocates memory for a job structure with some members initiallized
// returns NULL if no enough memory
job * new_job(pid_t pid, const char *command, enum job_state state)
{
    job * aux;
    aux = (job *) malloc(sizeof(job));
    aux->pgid = pid;
    aux->state = state;
    aux->command = command? strdup(command): NULL;
    // Initiallize new fields if required
    aux->argv = NULL;
    return aux;
}
// -----------------------------------------------------------------------------
// print data of the n-th job: [index] pid, command, status ...
void print_job(job *this, unsigned int index)
{
    printf("<%d>\t[%d]\t(%s)", index, this->pgid, this->command);
    // Show new fields if required
    char **p = this->argv;
    if (p) {
        printf("\tcmdline:");
        while (*p) printf(" %s", *p++);
    }
    printf("\tstate: %s\n", state_strings[this->state]);
}
// -----------------------------------------------------------------------------
// deallocate job struct previously allocated with new_job or malloc
void free_job(job *item)
{
    free_argv(item->argv);  // from parse_line.h
    free(item->command);
    free(item);
}


// -----------------------------------------------------------------------
// Example functions for LIST of JOBS (create your own ones if necessary)
// -----------------------------------------------------------------------
// returns the first element of the list which gpid match with the given arg
// or NULL if no one matches
job * get_job_bypid(list_head_t *list, pid_t pid)
{
    int check_job_pgid(void *arg)
    {
        if (arg == NULL) return 0;
        job *item = (job *)arg;
        return (item->pgid == pid);
    }
    return (job *)get_item_byfunc(list, &check_job_pgid);
}

// returns the first element of the list which command match with the given arg
// or NULL if no one matches
job * get_job_bycmd(list_head_t *list, char *cmd)
{
    int check_job_cmd(void *arg)
    {
        if (arg == NULL) return 0;
        job *item = (job *)arg;
        return (strcmp(item->command, cmd) == 0);
    }
    return (job *)get_item_byfunc(list, check_job_cmd);
}
// -----------------------------------------------------------------------------
// traverses the list applying the print_job function to each element
void print_job_list(list_head_t *this)
{
    int n = 1;
    printf("Contents of %s (%d jobs):\n", this->name, this->count);
    traverse_list(this, (void(*)(void*,unsigned int))print_job);
}
// -----------------------------------------------------------------------------



