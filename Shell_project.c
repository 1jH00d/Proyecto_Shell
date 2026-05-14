//------------------------------------------------------------------------------
// UNIX Shell Project
// 
// Sistemas Operativos
// Dept. Arquitectura de Computadores - UMA
// 
// To compile and run the program:
//    $ gcc Shell_project.c parse_line.c list.c job_control.c -o Shell
//    $ ./Shell          
//    ShellSO > 
//     (then type ^D to exit program)
//------------------------------------------------------------------------------

// standard headers
#include <stdio.h>          // printf, stderr, perror, fprintf
#include <stdlib.h>         // malloc, free
//#include <malloc.h>
#include <string.h>         // strcmp
#include <fcntl.h>          // open
#include <unistd.h>         // fork, execvp, tcgetpgrp, dup2, close
//#include <termios.h>
#include <signal.h>         // signal
#include <sys/wait.h>       // waitpid
//#include <sys/types.h>
#include <errno.h>          // errno

// local project headers
#include "parse_line.h"     // link with parse_line.o
#include "job_control.h"    // link with job_control.o and list.o


// -----------------------------------------------------------------------------
//                            Global data structures
// -----------------------------------------------------------------------------
// Declara aqui las variables globales que tengan que ser accedidas desde los
//  manejadores establecidos con signal() o sigaction()


// -----------------------------------------------------------------------------
// Useful functions to deal with signal handlers and signal masks
// -----------------------------------------------------------------------------
// set a handler (SIG_IGN or SIG_DFL) for signal sent by terminal
void terminal_signals(void (*func)(int))
{
    signal(SIGINT,  func); // crtl+c interrupt tecleado en el terminal
    signal(SIGQUIT, func); // ctrl+\ quit tecleado en el terminal
    signal(SIGTSTP, func); // crtl+z Stop tecleado en el terminal
    signal(SIGTTIN, func); // proceso en segundo plano quiere leer del terminal
    signal(SIGTTOU, func); // proceso en segundo plano quiere escribir en el terminal
}
// -----------------------------------------------------------------------------
// mask or unmask a given signal depending on the block argument
void mask_signal(int signal, int block)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, signal);
    sigprocmask(block, &mask, NULL); // block: SIG_BLOCK/SIG_UNBLOCK
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//                            MAIN          
// -----------------------------------------------------------------------------
int main(void)
{
    char **argv = NULL;
    int argc;
    // probably useful variables:
    int background;             // equals 1 if a command is followed by '&'
    int pid_fork, pid_wait;     // pid for created and waited process
    int wstatus;                // status returned by waitpid
    char *file_in, *file_out;   // for redirections

    int shell_pid = getpid(); // getpid() nos da el PID de nuestro propio programa
    int last_pid = 0;         // Al principio no hay un proceso anterior
    int retval = 0;           // Al principio el estado es 0

    while (1) {
        free_argv(argv);
        int ret = get_command("ShellSO > ", &argc, &argv);
        if (ret == -1) exit(EXIT_FAILURE);      // error in read(2)
        if (ret == 0) break;                    // finish loop if ^D (eof)
        if (argc == 0) continue;                // empty command: next iteration
        argc = parse_comments(argv);
        if (argc == 0) continue; // empty command after parsing comment #
        argc = parse_background(argv, &background);
        if (argc == 0) continue; // empty command after parsing background &
        subs_autovars(argv, shell_pid, last_pid, retval);
        argc = parse_redirections(argv,  &file_in, &file_out);
        if (argc == 0) continue; // empty command after parsing redirections
        parse_escape(argv);

        pid_fork = fork();

        if ( pid_fork < 0 ){
             perror("Error en fork");

        }else if ( pid_fork == 0 ){                       // Zona del Hijo
            
            execvp(argv[0],argv);

            perror(argv[0]); // Imprime el error [cite: 115]
            exit(EXIT_FAILURE); // El hijo debe morir si falla execvp

        }else{                                           // Zona del Padre   
            
            if ( background == 0 ){
                waitpid(pid_fork, &wstatus, 0);

                if (WIFEXITED(wstatus)){

                    printf("[%d] (%s) Terminated with status: %d\n", pid_fork, argv[0], WEXITSTATUS(wstatus));

                }else if (WIFSIGNALED(wstatus)){
                    printf("[%d] (%s) Signaled by signal: %d\n", pid_fork, argv[0], WTERMSIG(wstatus));

                }
        
            }else{
                printf("[%d] (%s) Running in Background\n", pid_fork, argv[0]);
            }
        }

        // the steps are:
        // (1) fork a child process using fork()
        // (2) the child process will invoke execvp()
        // (3) if background == 0, the parent will wait, otherwise
        // (4) Shell shows a status message for processed command 
        // (5) loop ret

    } // end while
    printf("\nBye\n");
    return 0;
}

