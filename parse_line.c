// -----------------------------------------------------------------------------
// UNIX Shell Project
// command line parsing
// -----------------------------------------------------------------------------
// Nombre: Jorge Barrios Lara
// Asignatura: Sistemas Operativos 2025/2026

#include <stdlib.h>     // malloc, realloc, free
#include <stdio.h>      // perror, fprintf(debug)
#include <string.h>     // strdup, strcmp
#include <unistd.h>     // read, STDIN_FILENO
#include "parse_line.h" // check prototypes match implementation

// -----------------------------------------------------------------------------
//  get_command() reads in the next command line, separating it into distinct
//  tokens using whitespace as delimiters.
//  Input arguments:
//      prompt: the string to be shown before reading the command line
//  Output arguments:
//      argc: the number of separated words
//      argv: allocated vector of pointers to allocated strings (words)
//  Return:
//         1: the line was readed and procesed (perhaps 0 argv: see argc)
//         0: ^d was entered, end of user command stream (or eof input stream)
//        -1: in case of error in read(2)
//
//  Example of use:
//     ...
//     int argc;
//     char **argv;
//     while(...) { // Shell main loop
//          ...
//          int ret = get_command(&argc, &argv);
//          if (ret == -1) break;
//          if (ret == 0) break;
//          if (argc == 0) continue;
//          ...
// -----------------------------------------------------------------------------
int get_command(char *prompt, int *argc, char ***pargv)
{
    char *command_line;         // command line buffer
    int allocated_space = 80;   // initial command_line space
    int free_space;
    int bytes_readed;
    int total_length;
    
    char **argv;
    int max_argv = 2;           // initial max. number of arguments
    int arg_count = 0;

    int i, arg_pos;
    int escaped;
    int eol = 0;                // end of line flag

    // init (output) arguments
    *argc = 0;
    *pargv = NULL;

    // show prompt
    printf("%s",prompt);
    fflush(stdout);
    
    // alloc space to read command line
    command_line = malloc(allocated_space);

    // read what the user enters on the command line
    // returns the number of bytes read is returned (zero indicates end of file)
    // On error, -1 is returned, and errno is set to indicate the error
    free_space = allocated_space;
    total_length = 0;
    while (1) {
        bytes_readed = read(STDIN_FILENO, &command_line[total_length], free_space);
        if (bytes_readed == 0) { // ^d was entered, end of user command stream
            free(command_line);
            return 0;
        }
        if (bytes_readed < 0) { 
            perror("get_command: read");
            free(command_line);
            return -1;
        }
        total_length += bytes_readed;
        if (command_line[total_length-1] == '\n') {
			if (command_line[total_length-2] == '\\') {	// crop line
				total_length -= 2;
			} else {							// end of command line
				command_line[total_length-1] = '\0';
				break;
			}
		}
        // buffer exhausted, realloc space and go on reading
        free_space -= bytes_readed;
        if (free_space == 0) {
            int extra_space = allocated_space;
            char *newBuffer = realloc(command_line, allocated_space + extra_space);
            if (newBuffer == NULL) {
                perror("get_command: realloc input buffer");
                return -1;
            }
            command_line = newBuffer;
            allocated_space += extra_space;
            free_space += extra_space;
        }
    }

    // alloc space for argument vector and initallize to NULL
    argv = malloc(max_argv*sizeof(char *));
    for (i = 0; i < max_argv; i++) argv[i] = NULL;

    // parse command_line
    arg_pos = -1;
    escaped = 0;
    for (i = 0; i < total_length && !eol; i++) { 
        switch (command_line[i]) {
        case '\\':
            escaped = !escaped;
            if (arg_pos == -1) arg_pos = i; // position of argument
            break;
        case '\0':                  // should be the last char examined
            eol = 1;                // pass through
        case ' ':                   // pass through
        case '\t':                  // argument separators
            if (!escaped) {
                command_line[i] = '\0'; // add a null char; make a C string
                if (arg_pos != -1) {
                    argv[arg_count] = strdup(&command_line[arg_pos]); // copy string
                    arg_count++;
                    if (arg_count+1 >= max_argv) {
                        max_argv = (arg_count+1)*2;
                        char *ptr = realloc(argv, max_argv*sizeof(char *));
                        if (ptr == NULL) {
                            perror("get_command: realloc arguments");
                            free(command_line);
                            free_argv(argv);
                            return -1;
                        }
                        argv = (char **)ptr;
                    }
                }
                arg_pos = -1;
                break;
            }
            // else pass through
        default:
            if (arg_pos == -1) arg_pos = i; // position of argument
            escaped = 0;
        }  // end switch
    }  // end for
    argv[arg_count] = NULL;

    free(command_line);

    *pargv = argv;
    *argc = arg_count;
    return 1;
}

// -----------------------------------------------------------------------------
// deallocate argv
// -----------------------------------------------------------------------------
void free_argv(char **argv)
{
    if (!argv) return;
    char **p = argv;
    while (*p) free(*p++);
    free(argv);
}

// -----------------------------------------------------------------------------
// Parse out comments after #
// Example of use:
//
//     while(...) { // Shell main loop
//          ...
//          ret = get_command(...);
//          ...
//          argc = parse_comments(argv);
//          if (argc == 0) continue;
//          ...
// -----------------------------------------------------------------------------
int parse_comments(char **argv)
{
    int argc = 0;
    while (*argv && **argv != '#') {
        argv++;
        argc++;
    }
    while (*argv) {
        free(*argv);
        *argv = NULL;
        argv++;
    }
    return argc;
}

// -----------------------------------------------------------------------------
// Parse out background symbol: &
//      Atention:   this remove every character and argument after '&'
// Example of use:
//
//     while(...) { // Shell main loop
//          ...
//          argc = parse_comments(...);
//          ...
//          int background;
//          argc = parse_background(argv, &background);
//          if (argc == 0) continue;
//          ...
// -----------------------------------------------------------------------------
int parse_background(char **argv, int *background)
{
    int argc = 0;
    char *p, found = 0;
    *background = 0;
    while (*argv && **argv != '&' && !found) { // if '&' is the first char, this argv must be freed
        for (p = *argv; *p; p++) {
            if (*p == '&') {
                found = 1;
                *p = '\0';
                break;
            }
            if (*p == '\\') p++;    // skip next char
        }
        argv++;
        argc++;
    }
    if (*argv || found) *background = 1;
    while (*argv) {
        free(*argv);
        *argv = NULL;
        argv++;
    }
    return argc;
}


int subs_autovars(char **argv, int shell_pid, int last_pid, int retval)
{
    int i = 0;
    
    // Recorremos todos los argumentos hasta encontrar el final (NULL)
    while (argv[i] != NULL) {
        
        // Comprobamos si el argumento es la variable $$
        if (strcmp(argv[i], "$$") == 0) {
            free(argv[i]);                 // Liberamos la cadena original "$$" 
            argv[i] = malloc(32);          // Pedimos memoria nueva (32 bytes es de sobra para un número) 
            sprintf(argv[i], "%d", shell_pid); // Escribimos el PID del shell como texto en esa memoria
        }
        // Comprobamos si el argumento es la variable $!
        else if (strcmp(argv[i], "$!") == 0) {
            free(argv[i]);                 // 
            argv[i] = malloc(32);          // 
            sprintf(argv[i], "%d", last_pid); 
        }
        // Comprobamos si el argumento es la variable $?
        else if (strcmp(argv[i], "$?") == 0) {
            free(argv[i]);                 // 
            argv[i] = malloc(32);          // 
            sprintf(argv[i], "%d", retval);
        }
        
        i++;
    }
    
    return i; // Retornamos el número de argumentos [cite: 442]
}


// -----------------------------------------------------------------------------
// Parse redirections operators '<' '>' once argv structure has been built.
// Example of use:
//
//     while(...) { // Shell main loop
//          ...
//          argc = parse_background(...);
//          ...
//          char *file_in, *file_out;
//          argc = parse_redirections(argv, &file_in, &file_out);
//          if (argc == 0) continue;
//          ...
//
// For a valid redirection, a blank space is required before and after
// redirection operators '<' or '>'.
// -----------------------------------------------------------------------------
int parse_redirections(char **argv,  char **file_in, char **file_out)
{
    *file_in = NULL;
    *file_out = NULL;
    char **argv_start = argv;
    int argc = 0;
    while (*argv) {
        int is_in = !strcmp(*argv, "<");
        int is_out = !strcmp(*argv, ">");
        if (is_in || is_out) {
            free(*argv);
            argv++;
            if (*argv) {
                if (is_in) {
                    if (*file_in) {
                        fprintf(stderr, "too many input redirections: %s %s, keeping: %s\n",
                                        *file_in, *argv, *file_in);
                        free(*argv);
                    } else {
                        *file_in = *argv;
                    }
                }
                if (is_out) {
                    if (*file_out) {
                        fprintf(stderr, "too many output redirections: %s %s, keeping: %s\n",
                                        *file_out, *argv, *file_out);
                        free(*argv);
                    } else {
                        *file_out = *argv;
                    }
                }
                char **aux = argv + 1;
                while (*aux) {
                   *(aux-2) = *aux;
                   aux++;    
                }
                *(aux-2) = NULL;
                argv--;
            } else {
                /* Syntax error */
                fprintf(stderr, "syntax error in redirection\n");
                argv = argv_start;
                while (*argv) {
                    free(*argv);
                    *argv = NULL;
                    argv++;
                }
                argv_start[0] = NULL; // Do nothing
                argc = 0;
            }
        } else {
            argv++;
            argc++;
        }
    }
    // Debug:
    // *file_in && fprintf(stderr, "[parse_redirections] file_in='%s'\n", *file_in);
    // *file_out && fprintf(stderr, "[parse_redirections] file_out='%s'\n", *file_out);
    return argc;
}


// -----------------------------------------------------------------------------
// Clean up of escape character '\'
// Example of use:
//
//     while(...) { // Shell main loop
//          ...
//          char *file_in, *file_out;
//          argc = parse_redirections(argv, &file_in, &file_out);
//          if (argc == 0) continue;
//          parse_escape(argv);
//          ...
// -----------------------------------------------------------------------------
int parse_escape(char **argv)
{
    int argc = 0;
    char *p, *q;
    while (*argv) {
		for (p = q = *argv; *q; p++, q++) {
            if (*q == '\\') q++;
            *p = *q;
        }
        *p = *q;    // copy null char
        argv++;
        argc++;
    }
    return argc;
}



// =============================================================================
// alarm-proc - temporizador basado en procesos
// =============================================================================


/*
if (strcmp(argv[0], "alarm-proc") == 0) {
    if (argc < 3 || atoi(argv[1]) <= 0) {
        printf("alarm-proc: uso: alarm-proc <segundos> <comando> [args...]\n");
        continue;
    }
    int seconds = atoi(argv[1]);
    char **args = &argv[2];

    pid_fork = fork();   // fork del proceso del comando
    if (pid_fork < 0) {
        perror("fork");
    } else if (pid_fork == 0) {
        setpgid(0, 0);
        terminal_signals(SIG_DFL);
        execvp(args[0], args);
        perror(args[0]);
        exit(EXIT_FAILURE);
    } else {
        setpgid(pid_fork, pid_fork);

        // fork del proceso temporizador
        int pid_timer = fork();
        if (pid_timer == 0) {
            setpgid(0, 0);                    // su propio grupo
            signal(SIGCHLD, SIG_DFL);
            sleep(seconds);
            killpg(pid_fork, SIGKILL);        // matar el proceso original
            killpg(pid_fork, SIGCONT);        // por si estuviera suspendido
            exit(0);
        }
        // el temporizador será recogido por el manejador SIGCHLD

        if (background == 0) {
            tcsetpgrp(STDIN_FILENO, pid_fork);
            waitpid(pid_fork, &wstatus, WUNTRACED);
            tcsetpgrp(STDIN_FILENO, getpid());
            if (WIFEXITED(wstatus))
                printf("[%d] (%s) Terminated with status: %d\n",
                       pid_fork, args[0], WEXITSTATUS(wstatus));
            else if (WIFSIGNALED(wstatus))
                printf("[%d] (%s) Signaled by signal: %d\n",
                       pid_fork, args[0], WTERMSIG(wstatus));
            else if (WIFSTOPPED(wstatus)) {
                mask_signal(SIGCHLD, SIG_BLOCK);
                add_job(job_list, new_job(pid_fork, args[0], STOPPED));
                mask_signal(SIGCHLD, SIG_UNBLOCK);
                printf("[%d] (%s) Stopped by signal: %d\n",
                       pid_fork, args[0], WSTOPSIG(wstatus));
            }
        } else {
            mask_signal(SIGCHLD, SIG_BLOCK);
            add_job(job_list, new_job(pid_fork, args[0], BACKGROUND));
            mask_signal(SIGCHLD, SIG_UNBLOCK);
            printf("[%d] (%s) Running in Background\n", pid_fork, args[0]);
        }
    }
    continue;
}
*/


// =============================================================================
// nuevo modo con símbolo % (modo "silencioso")
// =============================================================================

/*
// En la sección de detección de símbolos (antes de comandos internos):
int silent_mode = 0;
// resetear al inicio del bucle: silent_mode = 0;

if (argc > 0 && strcmp(argv[argc - 1], "%") == 0) {
    silent_mode = 1;
    background = 1;
    free(argv[argc - 1]);
    argv[argc - 1] = NULL;
    argc--;
}

// En el bloque background del padre:
if (background == 1) {
    mask_signal(SIGCHLD, SIG_BLOCK);
    add_job(job_list, new_job(pid_fork, argv[0], BACKGROUND));
    mask_signal(SIGCHLD, SIG_UNBLOCK);
    if (!silent_mode) {
        printf("[%d] (%s) Running in Background\n", pid_fork, argv[0]);
    }
}
*/


// =============================================================================
// manejador SIGUSR1 - imprimir lista de jobs al recibir señal
// =============================================================================


/*
void manejador_usr1(int sig) {
    mask_signal(SIGCHLD, SIG_BLOCK);
    print_job_list(job_list);
    mask_signal(SIGCHLD, SIG_UNBLOCK);
}

// En main antes del bucle:
signal(SIGUSR1, manejador_usr1);
*/


// =============================================================================
// manejador SIGHUP - escribir en fichero al recibir SIGHUP
// =============================================================================
/*
void manejador_hup(int sig) {
    FILE *fp = fopen("shell_log.txt", "a");
    if (fp) {
        fprintf(fp, "SIGHUP recibido. Jobs activos: %d\n", job_list->count);
        for (int i = 1; i <= job_list->count; i++) {
            job *j = get_job_bypos(job_list, i);
            fprintf(fp, "  [%d] %s %s\n", j->pgid, j->command,
                    state_strings[j->state]);
        }
        fclose(fp);
    }
}

// En main antes del bucle:
signal(SIGHUP, manejador_hup);
*/


// =============================================================================
// thread contador - thread que imprime info cada N segundos
// =============================================================================

/*
typedef struct {
    int interval;   // cada cuántos segundos imprimir
} monitor_data_t;

void *monitor_thread(void *arg) {
    monitor_data_t *data = (monitor_data_t *)arg;
    while (1) {
        sleep(data->interval);
        // acceso a job_list desde el thread: no bloquear SIGCHLD aquí
        // porque sigprocmask solo afecta al thread principal en algunos sistemas
        printf("[monitor] Jobs activos: %d\n", job_list->count);
        fflush(stdout);
    }
    return NULL;
}

// Crear en main antes del bucle:
monitor_data_t *mdata = malloc(sizeof(monitor_data_t));
mdata->interval = 5;
pthread_t monitor_tid;
pthread_attr_t mattr;
pthread_attr_init(&mattr);
pthread_attr_setdetachstate(&mattr, PTHREAD_CREATE_DETACHED);
pthread_create(&monitor_tid, &mattr, monitor_thread, mdata);
pthread_attr_destroy(&mattr);
*/


// =============================================================================
// modo respawnable con límite de intentos
// =============================================================================

/*
// En job_control.h, añadir a la estructura job:
//   int max_respawn;     // máximo de relanzamientos (-1 = infinito)
//   int respawn_count;   // cuántas veces se ha relanzado

// En el manejador, al detectar WIFEXITED de un job RESPAWNABLE:
if (current_job->state == RESPAWNABLE) {
    if (current_job->max_respawn == -1 ||
        current_job->respawn_count < current_job->max_respawn) {
        
        current_job->respawn_count++;
        int new_pid = fork();
        if (new_pid == 0) {
            setpgid(0, 0);
            terminal_signals(SIG_DFL);
            execvp(current_job->argv[0], current_job->argv);
            exit(EXIT_FAILURE);
        } else {
            setpgid(new_pid, new_pid);
            current_job->pgid = new_pid;
        }
    } else {
        // agotados los intentos, comportamiento normal
        printf("[%d] (%s) Max respawn reached\n",
               current_job->pgid, current_job->command);
        del_job(job_list, current_job);
        free_job(current_job);
    }
}
*/


// =============================================================================
// alarm-thread con señal configurable
// =============================================================================

/*
typedef struct {
    int seconds;
    int signal;     // señal a mandar
    pid_t pgid;
} alarm_sig_data_t;

void *alarm_sig_thread(void *arg) {
    alarm_sig_data_t *data = (alarm_sig_data_t *)arg;
    sleep(data->seconds);
    killpg(data->pgid, data->signal);
    if (data->signal != SIGKILL && data->signal != SIGCONT)
        killpg(data->pgid, SIGCONT);  // por si suspendido
    free(data);
    return NULL;
}

// En el comando:
if (strcmp(argv[0], "alarm-signal") == 0) {
    if (argc < 4) { printf("alarm-signal: error de sintaxis\n"); continue; }
    int seconds = atoi(argv[1]);
    int sig     = atoi(argv[2]);
    char **args = &argv[3];
    // ... fork del hijo, crear thread con alarm_sig_data_t ...
}
*/
