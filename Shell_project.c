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
// Nombre: Jorge Barrios Lara
// Asignatura: Sistemas Operativos 2025/2026

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
// manejador (SIGCHLD handler)
// Se ejecuta automáticamente cada vez que un proceso hijo cambia de estado
// (termina, se suspende o se reanuda). Su función es:
//   1. Recorrer la lista de jobs y comprobar cuál cambió de estado.
//   2. Actualizar la lista: borrar el job si terminó, o cambiar su estado.
//   3. Evitar procesos zombie llamando a waitpid con WNOHANG.
//
// Se usa WNOHANG para que waitpid no bloquee si el proceso no ha cambiado.
// Se usa WUNTRACED para detectar suspensiones.
// Se usa WCONTINUED para detectar reanudaciones.
//
// La señal SIGCHLD se bloquea durante la ejecución del manejador para evitar
// que una segunda señal interrumpa el recorrido de la lista.
// ------------------------------------------------------------------------------
void manejador(int signal)
{
    job *current_job;
    int wstatus;
    int pid_ret;
 
    // Bloqueamos SIGCHLD para proteger el acceso a la lista
    mask_signal(SIGCHLD, SIG_BLOCK);
 
    int i = 1;
    while (i <= job_list->count) {
        current_job = get_job_bypos(job_list, i);
        pid_ret = waitpid(current_job->pgid, &wstatus,
                          WNOHANG | WUNTRACED | WCONTINUED);
 
        if (pid_ret == current_job->pgid) {
            // Este job cambió de estado
 
            if (WIFEXITED(wstatus)) {
                // El proceso terminó normalmente con exit()
                printf("[%d] (%s) Terminated with status: %d\n",
                       current_job->pgid, current_job->command,
                       WEXITSTATUS(wstatus));
                del_job(job_list, current_job);
                free_job(current_job);
                // No incrementamos i: el siguiente ocupa esta posición
 
            } else if (WIFSIGNALED(wstatus)) {
                // El proceso terminó al recibir una señal (ej. SIGKILL)
                printf("[%d] (%s) Signaled by signal: %d\n",
                       current_job->pgid, current_job->command,
                       WTERMSIG(wstatus));
                del_job(job_list, current_job);
                free_job(current_job);
                // No incrementamos i: el siguiente ocupa esta posición
 
            } else if (WIFSTOPPED(wstatus)) {
                // El proceso se suspendió (ej. SIGTSTP, SIGTTIN)
                printf("[%d] (%s) Stopped by signal: %d\n",
                       current_job->pgid, current_job->command,
                       WSTOPSIG(wstatus));
                current_job->state = STOPPED;
                i++;
 
            } else if (WIFCONTINUED(wstatus)) {
                // El proceso se reanudó al recibir SIGCONT
                printf("[%d] (%s) Continued\n",
                       current_job->pgid, current_job->command);
                current_job->state = BACKGROUND;
                i++;
            }
        } else {
            // Este job no cambió de estado, pasamos al siguiente
            i++;
        }
    }
 
    // Desbloqueamos SIGCHLD al salir 
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

        // ---------------------------------------------------------------------
        // Comandos internos: el shell los ejecuta directamente sin hacer fork
        // ---------------------------------------------------------------------
 
        // Comando interno: cd
        // Cambia el directorio de trabajo del shell usando chdir()
    
        if (strcmp(argv[0], "cd") == 0) {

            char *curdir, curdirbuf[PATH_MAX_LEN+1];
            curdir = getcwd(curdirbuf, PATH_MAX_LEN);

            if (argv[1] == NULL) {
                char *p = getenv("HOME");
                if (p && chdir(p) == 0) {
                    setenv("OLDPWD", curdir, 1);
                }

            } else if (strcmp(argv[1], "-") == 0) {
                char *p = getenv("OLDPWD");
                if (p == NULL || strcmp(p, "") == 0) {
                    printf("No habia directorio anterior\n");
                } else {
                    if (chdir(p) == 0) {
                        if (curdir) setenv("OLDPWD", curdir, 1);
                    }
                }

            } else {
                if (chdir(argv[1]) == 0) {
                    if (curdir) setenv("OLDPWD", curdir, 1);
                }
            }

            continue;
        }

        // Comando interno: exit
        // Sale del Shell

        if ( strcmp(argv[0], "exit") == 0){

            int exit_status = 0;

            if (argv[1] != NULL) {
                char *endptr;
                long valor = strtol(argv[1], &endptr, 10);
                if (*endptr == '\0') {
                    // el argumento era un número entero válido
                    exit_status = (int)valor;
                }
            }
            exit(exit_status);
            
            continue;
        }

        // Comando interno: lanzabg
        // Actua de forma identica a colocar un & al final del comando

        if ( strcmp(argv[0], "lanzabg") == 0){

            if (argv[1] == NULL) {
                printf("lanzabg: falta el comando\n");
                continue;
            }
            char **args1 = &argv[1];

            pid_fork = fork();

            if ( pid_fork < 0 ){
                perror("Error en fork");

            }else if ( pid_fork == 0 ){                       // Zona del Hijo

                // Creamos un nuevo grupo de procesos para este hijo.
                // Así el shell y el comando son grupos independientes y las señales
                // del terminal solo afectan al grupo que lo tiene asignado.

                setpgid(0,0);

                // Restauramos las señales del terminal
                terminal_signals(SIG_DFL);

                // Redirección de entrada estándar desde fichero

                if (file_in != NULL) {
                    int fd = open(file_in, O_RDONLY);
                    if (fd == -1) { perror(file_in); exit(EXIT_FAILURE); }
                    dup2(fd, STDIN_FILENO);   // stdin ahora lee del fichero
                    close(fd);
                }

                // Redirección de salida estándar a fichero

                if (file_out != NULL) {
                    int fd = open(file_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd == -1) { perror(file_out); exit(EXIT_FAILURE); }
                    dup2(fd, STDOUT_FILENO);   // stdout ahora escribe en el fichero
                    close(fd);
                }
                

                // Sustituimos el código del hijo por el del programa a ejecutar
                execvp(args1[0],args1);

                perror(args1[0]); // Imprime el error [cite: 115]
                exit(EXIT_FAILURE); // El hijo debe morir si falla execvp

            }else{                                           // Zona del Padre
                
                setpgid(pid_fork, pid_fork);         // race condition: también en el parent
                // BACKGROUND: no cedemos el terminal, el shell sigue activo
                // Añadimos el job a la lista para poder gestionarlo después
                mask_signal(SIGCHLD, SIG_BLOCK);
                add_job(job_list, new_job(pid_fork,argv[0],BACKGROUND));
                mask_signal(SIGCHLD, SIG_UNBLOCK);
                printf("[%d] (%s) Running in Background\n", pid_fork, args1[0]);

                }
            continue;
        }



        // Comando interno: jobs
        // Muestra la lista de tareas en segundo plano o suspendidas

        if (strcmp(argv[0], "jobs") == 0) {

            mask_signal(SIGCHLD, SIG_BLOCK);
            print_job_list(job_list);
            mask_signal(SIGCHLD, SIG_UNBLOCK);
            continue;
        }

        // Comando interno: fg [n]
        // Lleva una tarea (por posición n, o la más reciente si no hay argumento)
        // a primer plano. Si estaba suspendida, le envía SIGCONT para reanudarla.
        // El shell cede el terminal al job y espera a que termine o se suspenda.

        if (strcmp(argv[0], "fg") == 0) {
            job *working_job;

            // Seleccionar el job: por posición o el más reciente (posición 1, LIFO)

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

            // Lo eliminamos de la lista antes de ponerlo en foreground

            mask_signal(SIGCHLD, SIG_BLOCK);
            del_job(job_list, working_job);  // Elimino el job de la lista
            mask_signal(SIGCHLD, SIG_UNBLOCK);

            tcsetpgrp(STDIN_FILENO, working_job->pgid);              // Cedo terminal
            working_job->state = FOREGROUND;                         // Cambio estado
            killpg(working_job->pgid, SIGCONT);                     // Enviamos SIGCONT por si estaba suspendido

            waitpid(working_job->pgid, &wstatus, WUNTRACED);        // Esperamos a que termine o se suspenda (igual que un foreground normal)
            tcsetpgrp(STDIN_FILENO, getpid());                       // Recuperamos el terminal para el shell

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

        // Comando interno: bg [n]
        // Reanuda en segundo plano una tarea suspendida.
        // Si ya estaba en background, informa de ello.

        if (strcmp(argv[0], "bg") == 0) {
            job *working_job;

            // Seleccionar el job: por posición o el más reciente (posición 1, LIFO)

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

                // Estaba suspendido: lo reanudamos en segundo plano
                printf("[%d] (%s) Running in BACKGROUND\n", working_job->pgid, working_job->command);
                working_job->state = BACKGROUND;
                killpg(working_job->pgid, SIGCONT);
            } else if (working_job->state == BACKGROUND) {

                // Ya estaba en segundo plano
                printf("[%d] (%s) Already in BACKGROUND\n", working_job->pgid, working_job->command);
            }

            continue;
        }


        // ---------------------------------------------------------------------
        // Comando externo: creamos un proceso hijo para ejecutarlo
        // ---------------------------------------------------------------------


        pid_fork = fork();

        if ( pid_fork < 0 ){
             perror("Error en fork");

        }else if ( pid_fork == 0 ){                       // Zona del Hijo

            // Creamos un nuevo grupo de procesos para este hijo.
            // Así el shell y el comando son grupos independientes y las señales
            // del terminal solo afectan al grupo que lo tiene asignado.

            setpgid(0,0);

             // Restauramos las señales del terminal
            terminal_signals(SIG_DFL);

            // Redirección de entrada estándar desde fichero

            if (file_in != NULL) {
                int fd = open(file_in, O_RDONLY);
                if (fd == -1) { perror(file_in); exit(EXIT_FAILURE); }
                dup2(fd, STDIN_FILENO);   // stdin ahora lee del fichero
                close(fd);
            }

            // Redirección de salida estándar a fichero

            if (file_out != NULL) {
                int fd = open(file_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd == -1) { perror(file_out); exit(EXIT_FAILURE); }
                dup2(fd, STDOUT_FILENO);   // stdout ahora escribe en el fichero
                close(fd);
            }
            

            // Sustituimos el código del hijo por el del programa a ejecutar
            execvp(argv[0],argv);

            perror(argv[0]); // Imprime el error [cite: 115]
            exit(EXIT_FAILURE); // El hijo debe morir si falla execvp

        }else{                                           // Zona del Padre
            
            setpgid(pid_fork, pid_fork);         // race condition: también en el parent
            
            if ( background == 0 ){

                // FOREGROUND: cedemos el terminal al hijo y esperamos

                tcsetpgrp(STDIN_FILENO, pid_fork);   // ceder terminal al child

                waitpid(pid_fork, &wstatus, WUNTRACED);

                tcsetpgrp(STDIN_FILENO, getpid());   // recuperar terminal
                

                if (WIFEXITED(wstatus)){
                    
                    // Terminó normalmente
                    printf("[%d] (%s) Terminated with status: %d\n", pid_fork, argv[0], WEXITSTATUS(wstatus));

                }else if (WIFSIGNALED(wstatus)){

                    // Terminó al recibir una señal (ej. CTRL+C → SIGINT)
                    printf("[%d] (%s) Signaled by signal: %d\n", pid_fork, argv[0], WTERMSIG(wstatus));

                }else if (WIFSTOPPED(wstatus)) {

                    // Se suspendió (ej. CTRL+Z → SIGTSTP)
                    mask_signal(SIGCHLD, SIG_BLOCK);
                    add_job(job_list, new_job(pid_fork, argv[0], STOPPED));
                    mask_signal(SIGCHLD, SIG_UNBLOCK);
                    printf("[%d] (%s) Stopped by signal: %d\n", pid_fork, argv[0], WSTOPSIG(wstatus));
                }
        
            }else{

                // BACKGROUND: no cedemos el terminal, el shell sigue activo
                // Añadimos el job a la lista para poder gestionarlo después
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

