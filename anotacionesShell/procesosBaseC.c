// =============================================================================
// PLANTILLAS BASE PARA EL EXAMEN DE SHELL
// =============================================================================
// Estas plantillas cubren los patrones más comunes que pueden aparecer
// en el examen. Adaptar según el enunciado.
// =============================================================================


// =============================================================================
// PLANTILLA 1: FORK COMPLETO (foreground + background + redirecciones)
// =============================================================================
// Usar cuando el enunciado pide lanzar un comando externo.
// Copiar este bloque y adaptar argv, args, nombre del comando, etc.

/*
pid_fork = fork();

if (pid_fork < 0) {
    perror("Error en fork");

} else if (pid_fork == 0) {
    // ----- HIJO -----
    setpgid(0, 0);                  // nuevo grupo de procesos
    terminal_signals(SIG_DFL);      // restaurar señales del terminal

    // Redirección de entrada (si aplica)
    if (file_in != NULL) {
        int fd = open(file_in, O_RDONLY);
        if (fd == -1) { perror(file_in); exit(EXIT_FAILURE); }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    // Redirección de salida (si aplica)
    if (file_out != NULL) {
        int fd = open(file_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) { perror(file_out); exit(EXIT_FAILURE); }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    execvp(argv[0], argv);
    perror(argv[0]);
    exit(EXIT_FAILURE);

} else {
    // ----- PADRE -----
    setpgid(pid_fork, pid_fork);    // race condition: también en el padre

    if (background == 0) {
        // FOREGROUND
        tcsetpgrp(STDIN_FILENO, pid_fork);
        waitpid(pid_fork, &wstatus, WUNTRACED);
        tcsetpgrp(STDIN_FILENO, getpid());

        if (WIFEXITED(wstatus)) {
            printf("[%d] (%s) Terminated with status: %d\n",
                   pid_fork, argv[0], WEXITSTATUS(wstatus));
        } else if (WIFSIGNALED(wstatus)) {
            printf("[%d] (%s) Signaled by signal: %d\n",
                   pid_fork, argv[0], WTERMSIG(wstatus));
        } else if (WIFSTOPPED(wstatus)) {
            mask_signal(SIGCHLD, SIG_BLOCK);
            add_job(job_list, new_job(pid_fork, argv[0], STOPPED));
            mask_signal(SIGCHLD, SIG_UNBLOCK);
            printf("[%d] (%s) Stopped by signal: %d\n",
                   pid_fork, argv[0], WSTOPSIG(wstatus));
        }
    } else {
        // BACKGROUND
        mask_signal(SIGCHLD, SIG_BLOCK);
        add_job(job_list, new_job(pid_fork, argv[0], BACKGROUND));
        mask_signal(SIGCHLD, SIG_UNBLOCK);
        printf("[%d] (%s) Running in Background\n", pid_fork, argv[0]);
    }
}
continue;
*/


// =============================================================================
// PLANTILLA 2: THREAD (alarm-thread y variantes)
// =============================================================================
// Usar cuando el enunciado pide crear un thread auxiliar.

/*
// --- ESTRUCTURA DE DATOS PARA EL THREAD ---
typedef struct {
    int seconds;        // tiempo de espera
    pid_t pgid;         // grupo del proceso a controlar
    // añadir más campos si el enunciado lo requiere
} hilo_data_t;

// --- FUNCIÓN DEL THREAD ---
void *funcion_thread(void *arg) {
    hilo_data_t *data = (hilo_data_t *)arg;

    sleep(data->seconds);

    // hacer lo que pida el enunciado, por ejemplo matar el proceso:
    killpg(data->pgid, SIGKILL);
    killpg(data->pgid, SIGCONT);   // por si estuviera suspendido

    free(data);
    return NULL;
}

// --- CREACIÓN DEL THREAD (en el padre, después del fork) ---
hilo_data_t *data = malloc(sizeof(hilo_data_t));
data->seconds = N;
data->pgid = pid_fork;

pthread_t tid;
pthread_attr_t attr;
pthread_attr_init(&attr);
pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
pthread_create(&tid, &attr, funcion_thread, data);
pthread_attr_destroy(&attr);
*/

// VARIANTE: thread que termina si el proceso termina antes del timeout
/*
void *funcion_thread(void *arg) {
    hilo_data_t *data = (hilo_data_t *)arg;

    int elapsed = 0;
    while (elapsed < data->seconds) {
        sleep(1);
        elapsed++;
        // comprobar si el proceso sigue vivo
        if (killpg(data->pgid, 0) == -1 && errno == ESRCH) {
            free(data);
            return NULL;   // proceso ya terminó, thread termina también
        }
    }

    killpg(data->pgid, SIGKILL);
    killpg(data->pgid, SIGCONT);
    free(data);
    return NULL;
}
*/


// =============================================================================
// PLANTILLA 3: PROCESO TEMPORIZADOR (alarm-proc y variantes)
// =============================================================================
// Usar cuando el enunciado pide un proceso auxiliar en vez de thread.
// El proceso temporizador no puede quedar zombie.

/*
// Crear el proceso temporizador DESPUÉS de crear el proceso del comando
// y conocer su pid_fork.

// Opción A: añadir el temporizador a la lista de jobs para que el
// manejador SIGCHLD lo recoja cuando termine.
int pid_timer = fork();
if (pid_timer == 0) {
    // PROCESO TEMPORIZADOR
    setpgid(0, 0);           // su propio grupo, no recibe señales del terminal
    signal(SIGCHLD, SIG_DFL); // comportamiento por defecto
    
    sleep(N);                // esperar N segundos
    
    killpg(pid_fork, SIGKILL);   // matar el proceso original
    killpg(pid_fork, SIGCONT);   // por si estuviera suspendido
    
    exit(0);
}
// En el padre: el temporizador se recoge en el manejador SIGCHLD
// porque hacemos waitpid de todos los hijos con WNOHANG.
// Si no está en la lista, simplemente waitpid retorna su pid y se ignora.
// Para evitar zombie sin cambiar el manejador, usar:
setpgid(pid_timer, pid_timer);
// El manejador con WNOHANG | WUNTRACED | WCONTINUED lo recogerá
// automáticamente en la siguiente iteración.
*/


// =============================================================================
// PLANTILLA 4: DETECCIÓN DE NUEVO SÍMBOLO AL FINAL DE LÍNEA
// =============================================================================
// Usar cuando el enunciado introduce un nuevo símbolo como '+', '%', '!', etc.
// Colocar ANTES de los comandos internos, después del parse_redirections.

/*
int nuevo_modo = 0;
// resetear al inicio de cada iteración del bucle:
// nuevo_modo = 0;

// detectar el símbolo '+' (o el que pida el enunciado)
if (argc > 0 && strcmp(argv[argc - 1], "+") == 0) {
    nuevo_modo = 1;
    background = 1;             // normalmente implica background
    free(argv[argc - 1]);
    argv[argc - 1] = NULL;
    argc--;
}
*/


// =============================================================================
// PLANTILLA 5: DESPLAZAR ARGV (para comandos tipo lanzabg, mask, alarm-*)
// =============================================================================
// Usar cuando el comando interno tiene la forma:
//   mi_comando [opciones] <comando_real> [args...]
// y quieres que el fork de abajo ejecute el comando_real.

/*
// Si el comando real empieza en argv[N]:
char **args = &argv[N];   // args[0] es el comando real

// Si hay que eliminar los primeros N elementos del array:
for (int j = 0; j < argc - N; j++) {
    argv[j] = argv[j + N];
}
argv[argc - N] = NULL;
argc -= N;
// Ahora argv[0] es el comando real y puedes dejar caer al fork de abajo.
*/


// =============================================================================
// PLANTILLA 6: NUEVO MANEJADOR DE SEÑAL
// =============================================================================
// Usar cuando el enunciado pide instalar un manejador para una señal nueva.

/*
void mi_manejador(int sig) {
    // Si accede a job_list, bloquear SIGCHLD:
    mask_signal(SIGCHLD, SIG_BLOCK);
    
    // ... hacer lo que pida el enunciado ...
    
    mask_signal(SIGCHLD, SIG_UNBLOCK);
}

// En main, antes del bucle:
signal(SIGUSR1, mi_manejador);   // o la señal que pida el enunciado
*/


// =============================================================================
// PLANTILLA 7: COPIAR ARGV (para respawnable y similares)
// =============================================================================
// Usar cuando necesitas guardar una copia del argv para relanzar después.

/*
char **copy_argv(char **original) {
    if (original == NULL) return NULL;
    int count = 0;
    while (original[count] != NULL) count++;
    char **copy = malloc((count + 1) * sizeof(char *));
    for (int i = 0; i < count; i++) copy[i] = strdup(original[i]);
    copy[count] = NULL;
    return copy;
}
*/
