// =============================================================================
// PREGUNTAS PROBABLES DEL EXAMEN - SEÑALES, MODOS Y TEMPORIZADORES
// =============================================================================


// =============================================================================
// PREGUNTA 11: alarm-proc - temporizador basado en procesos
// =============================================================================
// Enunciado: "Implementar alarm-proc N <cmd> igual que alarm-thread pero
// usando un proceso auxiliar en vez de un thread."

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
// PREGUNTA 12: nuevo modo con símbolo % (modo "silencioso")
// =============================================================================
// Enunciado típico: "Añadir un nuevo modo indicado con '%' al final de línea
// que lance el comando en background sin imprimir mensajes."

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
// PREGUNTA 13: manejador SIGUSR1 - imprimir lista de jobs al recibir señal
// =============================================================================
// Enunciado típico: "Hacer que el shell imprima la lista de jobs cuando
// reciba la señal SIGUSR1."

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
// PREGUNTA 14: manejador SIGHUP - escribir en fichero al recibir SIGHUP
// =============================================================================
// Enunciado típico: "Hacer que cuando el shell reciba SIGHUP escriba
// en un fichero de log la lista de jobs activos."

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
// PREGUNTA 15: thread contador - thread que imprime info cada N segundos
// =============================================================================
// Enunciado típico: "Crear un thread que cada N segundos imprima el número
// de jobs activos en la lista."

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
// PREGUNTA 16: modo respawnable con límite de intentos
// =============================================================================
// Enunciado típico: "Modificar el modo respawnable para que solo relance
// el proceso un máximo de N veces."
// Requiere añadir campo 'intentos' a la estructura job.

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
// PREGUNTA 17: alarm-thread con señal configurable
// =============================================================================
// Enunciado típico: "Implementar alarm-signal N SIG <cmd> que mande la
// señal SIG al proceso tras N segundos en vez de siempre SIGKILL."

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
