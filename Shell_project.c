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

list_head_t *job_list;



// -----------------------------------------------------------------------------
// Useful functions to deal with signal handlers and signal masks

/*

void manejador(int signal){

    job *current_job;
    int wstatus;
    int pid_ret;

     mask_signal(SIGCHLD, SIG_BLOCK);

    for (int i = 0; i < list_size(job_list); i++)
    {
        current_job = get_item_bypos(job_list,i);
        pid_ret = waitpid(current_job->pgid, &wstatus, WNOHANG | WUNTRACED | WCONTINUED);

        if ( pid_ret == current_job->pgid){

            if (WIFEXITED(wstatus)){

                    printf("[%d] (%s) Terminated with status: %d\n", current_job->pgid, current_job->command, WEXITSTATUS(wstatus));
                    del_job(job_list, current_job);
                    free_job(current_job);

            }else if (WIFSIGNALED(wstatus)){
                    printf("[%d] (%s) Signaled by signal: %d\n", current_job->pgid, current_job->command, WTERMSIG(wstatus));
                    del_job(job_list, current_job);
                    free_job(current_job);

            }else if (WIFSTOPPED(wstatus)) {
                    printf("[%d] (%s) Stopped by signal: %d\n", current_job, current_job->command, WSTOPSIG(wstatus));
                    current_job->state = STOPPED;
            }else if (WIFCONTINUED(wstatus)) {
                printf("[%d] (%s) Continued\n",current_job->pgid, current_job->command);
                current_job->state = BACKGROUND;
            }
        }

    }

    mask_signal(SIGCHLD, SIG_UNBLOCK);
    
}

*/






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
void manejador(int signal){

    job *current_job;
    int wstatus;
    int pid_ret;

    mask_signal(SIGCHLD, SIG_BLOCK);

    int i = 1;
    while (i <= job_list->count) {
        current_job = get_job_bypos(job_list, i);
        pid_ret = waitpid(current_job->pgid, &wstatus, WNOHANG | WUNTRACED | WCONTINUED);

        if (pid_ret == current_job->pgid) {

            if (WIFEXITED(wstatus)) {
                printf("[%d] (%s) Terminated with status: %d\n", current_job->pgid, current_job->command, WEXITSTATUS(wstatus));
                del_job(job_list, current_job);
                free_job(current_job);
                // no incrementes i

            } else if (WIFSIGNALED(wstatus)) {
                printf("[%d] (%s) Signaled by signal: %d\n", current_job->pgid, current_job->command, WTERMSIG(wstatus));
                del_job(job_list, current_job);
                free_job(current_job);
                // no incrementes i

            } else if (WIFSTOPPED(wstatus)) {
                printf("[%d] (%s) Stopped by signal: %d\n", current_job->pgid, current_job->command, WSTOPSIG(wstatus));
                current_job->state = STOPPED;
                i++;

            } else if (WIFCONTINUED(wstatus)) {
                printf("[%d] (%s) Continued\n", current_job->pgid, current_job->command);
                current_job->state = BACKGROUND;
                i++;
            }
        } else {
            i++;
        }
    }


    mask_signal(SIGCHLD, SIG_UNBLOCK);
    
}

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

    job_list = new_list("jobs");

    
    terminal_signals(SIG_IGN);
    signal(SIGCHLD, manejador);

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

        if ( strcmp(argv[0], "cd") == 0){
            if (chdir(argv[1]) == -1) {
                perror(argv[1]);
            }
            continue;
        }

        if (strcmp(argv[0], "jobs") == 0) {

            mask_signal(SIGCHLD, SIG_BLOCK);
            print_job_list(job_list);
            mask_signal(SIGCHLD, SIG_UNBLOCK);
            continue;
        }

        if (strcmp(argv[0], "fg") == 0) {
            job *working_job;

            if (argv[1] != NULL) {
                working_job = get_job_bypos(job_list, atoi(argv[1]));
            } else {
                working_job = get_job_bypos(job_list, 1);  // tarea actual = la más reciente
            }

            if (working_job == NULL) {
                printf("fg: no jobs\n");
                continue;
            }

            // a partir de aquí, mismo código para los dos casos
            printf("[%d] (%s) Running in FOREGROUND\n", working_job->pgid, working_job->command);

            mask_signal(SIGCHLD, SIG_BLOCK);
            del_job(job_list, working_job);  // Elimino el job de la lista
            mask_signal(SIGCHLD, SIG_UNBLOCK);

            tcsetpgrp(STDIN_FILENO, working_job->pgid);              // Cedo terminal
            working_job->state = FOREGROUND;                         // Cambio estado
            killpg(working_job->pgid, SIGCONT);

            waitpid(working_job->pgid, &wstatus, WUNTRACED);
            tcsetpgrp(STDIN_FILENO, getpid());

            if (WIFEXITED(wstatus)) {
                printf("[%d] (%s) Terminated with status: %d\n", working_job->pgid, working_job->command, WEXITSTATUS(wstatus));
                free_job(working_job);
            } else if (WIFSIGNALED(wstatus)) {
                printf("[%d] (%s) Signaled by signal: %d\n", working_job->pgid, working_job->command, WTERMSIG(wstatus));
                free_job(working_job);
            } else if (WIFSTOPPED(wstatus)) {
                working_job->state = STOPPED;
                mask_signal(SIGCHLD, SIG_BLOCK);
                add_job(job_list, working_job);
                mask_signal(SIGCHLD, SIG_UNBLOCK);
                printf("[%d] (%s) Stopped by signal: %d\n", working_job->pgid, working_job->command, WSTOPSIG(wstatus));
            }

            continue;
        }


        if (strcmp(argv[0], "bg") == 0) {
            job *working_job;

            if (argv[1] != NULL) {
                working_job = get_job_bypos(job_list, atoi(argv[1]));
            } else {
                working_job = get_job_bypos(job_list, 1);
            }

            if (working_job == NULL) {
                printf("bg: no jobs\n");
                continue;
            }

            if (working_job->state == STOPPED) {
                printf("[%d] (%s) Running in BACKGROUND\n", working_job->pgid, working_job->command);
                working_job->state = BACKGROUND;
                killpg(working_job->pgid, SIGCONT);
            } else if (working_job->state == BACKGROUND) {
                printf("[%d] (%s) Already in BACKGROUND\n", working_job->pgid, working_job->command);
            }

            continue;
        }

        pid_fork = fork();

        if ( pid_fork < 0 ){
             perror("Error en fork");

        }else if ( pid_fork == 0 ){                       // Zona del Hijo

            setpgid(0,0);
            terminal_signals(SIG_DFL);

            if (file_in != NULL) {
                int fd = open(file_in, O_RDONLY);
                if (fd == -1) { perror(file_in); exit(EXIT_FAILURE); }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }

            if (file_out != NULL) {
                int fd = open(file_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd == -1) { perror(file_out); exit(EXIT_FAILURE); }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            
            execvp(argv[0],argv);

            perror(argv[0]); // Imprime el error [cite: 115]
            exit(EXIT_FAILURE); // El hijo debe morir si falla execvp

        }else{                                           // Zona del Padre
            
            setpgid(pid_fork, pid_fork);         // race condition: también en el parent
            
            if ( background == 0 ){

                tcsetpgrp(STDIN_FILENO, pid_fork);   // ceder terminal al child

                waitpid(pid_fork, &wstatus, WUNTRACED);

                tcsetpgrp(STDIN_FILENO, getpid());   // recuperar terminal
                

                if (WIFEXITED(wstatus)){

                    printf("[%d] (%s) Terminated with status: %d\n", pid_fork, argv[0], WEXITSTATUS(wstatus));

                }else if (WIFSIGNALED(wstatus)){
                    printf("[%d] (%s) Signaled by signal: %d\n", pid_fork, argv[0], WTERMSIG(wstatus));

                }else if (WIFSTOPPED(wstatus)) {
                    mask_signal(SIGCHLD, SIG_BLOCK);
                    add_job(job_list, new_job(pid_fork, argv[0], STOPPED));
                    mask_signal(SIGCHLD, SIG_UNBLOCK);
                    printf("[%d] (%s) Stopped by signal: %d\n", pid_fork, argv[0], WSTOPSIG(wstatus));
                }
        
            }else{
                mask_signal(SIGCHLD, SIG_BLOCK);
                add_job(job_list, new_job(pid_fork,argv[0],BACKGROUND));
                mask_signal(SIGCHLD, SIG_UNBLOCK);
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

