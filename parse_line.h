// -----------------------------------------------------------------------------
// UNIX Shell Project
// function prototypes and macros for line and argument parsing
// -----------------------------------------------------------------------------

#ifndef _PARSE_LINE_H
#define _PARSE_LINE_H

// -----------------------------------------------------------------------
//      PUBLIC FUNCTIONS
// -----------------------------------------------------------------------
// Functions for command line parsing:
int get_command(char *, int *, char ***);
void free_argv(char **);
int parse_comments(char **);
int parse_background(char **, int *);
int subs_autovars(char **argv, int shell_pid, int last_pid, int retval);
int parse_redirections(char **,  char **, char **);
int parse_escape(char **);
#endif

