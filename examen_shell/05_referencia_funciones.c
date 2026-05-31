// =============================================================================
// REFERENCIA RÁPIDA DE FUNCIONES DEL SISTEMA
// =============================================================================
// Para el examen: qué hace cada función, argumentos y valores de retorno.
// =============================================================================


// =============================================================================
// GESTIÓN DE PROCESOS
// =============================================================================

// fork() - crear un proceso hijo
// Retorna: pid del hijo al padre, 0 al hijo, -1 en error
pid_t pid = fork();

// execvp(cmd, argv) - sustituir imagen del proceso
// cmd: nombre del programa
// argv: array de argumentos terminado en NULL
// No retorna si tiene éxito; retorna -1 en error
execvp(argv[0], argv);

// waitpid(pid, &status, options) - esperar a un hijo
// pid > 0 : esperar ese pid concreto
// pid = -1: esperar cualquier hijo
// pid < -1: esperar cualquier hijo del grupo |pid|
// options: 0 (bloqueante), WNOHANG (no bloqueante), WUNTRACED (detectar stop),
//          WCONTINUED (detectar reanudación)
// Retorna: pid del proceso que cambió, 0 si WNOHANG y no cambió, -1 error
waitpid(pid_fork, &wstatus, WUNTRACED);
waitpid(-1, &wstatus, WNOHANG | WUNTRACED | WCONTINUED);  // cualquier hijo

// Macros para examinar wstatus:
WIFEXITED(wstatus)    // terminó con exit()  → usar WEXITSTATUS(wstatus)
WIFSIGNALED(wstatus)  // terminó por señal   → usar WTERMSIG(wstatus)
WIFSTOPPED(wstatus)   // se suspendió        → usar WSTOPSIG(wstatus)
WIFCONTINUED(wstatus) // se reanudó

// exit(status) - terminar el proceso con código de salida
exit(0);
exit(EXIT_FAILURE);  // = exit(1)


// =============================================================================
// GRUPOS DE PROCESOS Y TERMINAL
// =============================================================================

// setpgid(pid, pgid) - cambiar grupo de un proceso
// setpgid(0, 0): el proceso actual crea su propio grupo (líder)
// setpgid(pid, pid): hacer que pid sea líder de su propio grupo
setpgid(0, 0);               // en el hijo: crear nuevo grupo
setpgid(pid_fork, pid_fork); // en el padre: misma operación (race condition)

// getpgid(pid) - obtener el grupo de un proceso
// getpgid(0): grupo del proceso actual
pid_t pgid = getpgid(0);

// tcsetpgrp(fd, pgrp) - asignar el terminal a un grupo
// fd: descriptor del terminal (STDIN_FILENO)
// pgrp: grupo que recibirá el terminal
tcsetpgrp(STDIN_FILENO, pid_fork);  // ceder terminal al hijo
tcsetpgrp(STDIN_FILENO, getpid());  // recuperar terminal para el shell

// tcgetpgrp(fd) - consultar qué grupo tiene el terminal
pid_t current_fg = tcgetpgrp(STDIN_FILENO);


// =============================================================================
// SEÑALES
// =============================================================================

// signal(sig, handler) - instalar manejador
signal(SIGCHLD, manejador);   // instalar manejador
signal(SIGCHLD, SIG_IGN);     // ignorar señal
signal(SIGCHLD, SIG_DFL);     // comportamiento por defecto

// kill(pid, sig) - mandar señal a un proceso
// pid < 0: mandar al grupo |pid|
kill(pid_fork, SIGKILL);
kill(-pgid, SIGCONT);  // equivalente a killpg

// killpg(pgid, sig) - mandar señal a todo un grupo
killpg(pgid, SIGKILL);
killpg(pgid, SIGCONT);
killpg(pgid, SIGSTOP);
killpg(pgid, 0);        // comprobar si el grupo existe (sin matar)

// mask_signal(signal, block) - bloquear/desbloquear señal
// (función propia del shell, implementada con sigprocmask)
mask_signal(SIGCHLD, SIG_BLOCK);    // bloquear
mask_signal(SIGCHLD, SIG_UNBLOCK);  // desbloquear

// Señales importantes:
// SIGINT   (2)  CTRL+C  - terminar proceso
// SIGKILL  (9)  - terminar (no se puede ignorar)
// SIGTERM  (15) - terminar (se puede ignorar)
// SIGSTOP  (19) - suspender (no se puede ignorar)
// SIGTSTP  (20) CTRL+Z  - suspender (se puede ignorar)
// SIGCONT  (18) - reanudar proceso suspendido
// SIGCHLD  (17) - hijo cambió de estado
// SIGHUP   (1)  - terminal cerrado
// SIGUSR1  (10) - señal de usuario 1
// SIGUSR2  (12) - señal de usuario 2
// SIGTTIN  (21) - proceso bg intentó leer del terminal
// SIGTTOU  (22) - proceso bg intentó escribir en terminal


// =============================================================================
// REDIRECCIONES
// =============================================================================

// open(path, flags, mode) - abrir fichero
// Para lectura:
int fd_in = open(file_in, O_RDONLY);

// Para escritura (sobreescribir):
int fd_out = open(file_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);

// Para escritura (añadir al final):
int fd_app = open(file_out, O_WRONLY | O_CREAT | O_APPEND, 0644);

// dup2(oldfd, newfd) - duplicar descriptor
dup2(fd_in,  STDIN_FILENO);   // stdin  ahora lee del fichero
dup2(fd_out, STDOUT_FILENO);  // stdout ahora escribe en el fichero
dup2(fd_err, STDERR_FILENO);  // stderr ahora escribe en el fichero

// Siempre cerrar el descriptor original después de dup2:
close(fd_in);


// =============================================================================
// THREADS (pthread)
// =============================================================================

// Compilar con: gcc ... -lpthread

// Estructura para pasar datos al thread:
typedef struct {
    int campo1;
    pid_t campo2;
} mi_datos_t;

// Función del thread (siempre esta firma):
void *mi_funcion(void *arg) {
    mi_datos_t *data = (mi_datos_t *)arg;
    // ... hacer algo ...
    free(data);   // liberar la memoria que se pasó con malloc
    return NULL;
}

// Crear thread detached (se libera solo al terminar):
mi_datos_t *data = malloc(sizeof(mi_datos_t));
data->campo1 = valor1;
data->campo2 = valor2;

pthread_t tid;
pthread_attr_t attr;
pthread_attr_init(&attr);
pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
pthread_create(&tid, &attr, mi_funcion, data);
pthread_attr_destroy(&attr);


// =============================================================================
// GESTIÓN DE LA LISTA DE JOBS
// =============================================================================

// Siempre bloquear SIGCHLD al acceder a la lista:
mask_signal(SIGCHLD, SIG_BLOCK);
// ... operaciones con la lista ...
mask_signal(SIGCHLD, SIG_UNBLOCK);

// Crear job:
job *j = new_job(pid, comando, BACKGROUND);  // o STOPPED, FOREGROUND

// Añadir a la lista:
add_job(job_list, j);

// Obtener job:
job *j = get_job_bypos(job_list, 1);          // por posición (1 = más reciente)
job *j = get_job_bypid(job_list, pid);        // por pid

// Eliminar de la lista (NO libera memoria):
del_job(job_list, j);

// Liberar memoria del job:
free_job(j);

// Comprobar si está en la lista:
int pos = check_job(job_list, j);  // retorna posición o 0 si no está

// Imprimir toda la lista:
print_job_list(job_list);

// Estados posibles:
// FOREGROUND  - en primer plano (normalmente no está en la lista)
// BACKGROUND  - en segundo plano ejecutándose
// STOPPED     - suspendido


// =============================================================================
// DIRECTORIO DE TRABAJO Y ENTORNO
// =============================================================================

// getcwd - obtener directorio actual
char buf[4096];
char *curdir = getcwd(buf, 4096);  // retorna NULL si falla

// chdir - cambiar directorio
int ret = chdir(path);  // retorna 0 si OK, -1 si error

// getenv - obtener variable de entorno
char *home = getenv("HOME");   // NULL si no existe

// setenv - establecer variable de entorno
setenv("OLDPWD", curdir, 1);  // 1 = sobreescribir si ya existe

// Obtener PID propio:
int pid = getpid();


// =============================================================================
// SISTEMA DE FICHEROS /proc (para zjobs y similares)
// =============================================================================

// Recorrer /proc para encontrar procesos zombie hijos del shell:
void traverse_proc_zombie(int shell_pid) {
    DIR *d;
    struct dirent *dir;
    char buff[2048];
    d = opendir("/proc");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            sprintf(buff, "/proc/%s/stat", dir->d_name);
            FILE *fd = fopen(buff, "r");
            if (fd) {
                long pid, ppid;
                char state;
                // formato: pid (comando) estado ppid ...
                fscanf(fd, "%ld %s %c %ld", &pid, buff, &state, &ppid);
                fclose(fd);
                // Z = zombie, comprobar padre
                if (state == 'Z' && ppid == shell_pid) {
                    printf("%ld\n", pid);
                }
            }
        }
        closedir(d);
    }
}
// Requiere: #include <dirent.h>
