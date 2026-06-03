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
#include <dirent.h>         // este para poder hacer traverse_proc_zombie
#include <pthread.h>        // incluir los threads

// local project headers
#include "parse_line.h"     // link with parse_line.o
#include "job_control.h"    // link with job_control.o and list.o


// -----------------------------------------------------------------------------
//                            Global data structures
// -----------------------------------------------------------------------------
// Declara aqui las variables globales que tengan que ser accedidas desde los
//  manejadores establecidos con signal() o sigaction()

list_head_t *job_list;
#define PATH_MAX_LEN 4096

typedef struct {         //Estructura creada para el alarm-thread
    int seconds;
    pid_t pgid;
} alarm_data_t;




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
//Funcion para copiar el argv, para asi evitar problemas con el respawneable
// -----------------------------------------------------------------------------
char **copy_argv(char **original_argv) {
    if (original_argv == NULL) return NULL;

    // 1. Contamos cuántos argumentos hay en total
    int count = 0;
    while (original_argv[count] != NULL) {
        count++;
    }

    // 2. Reservamos memoria para el nuevo array de punteros (+1 para el NULL final)
    char **new_argv = malloc((count + 1) * sizeof(char *));
    if (new_argv == NULL) {
        perror("Error al reservar memoria para copia de argv");
        return NULL;
    }

    // 3. Duplicamos cada string individualmente usando strdup
    for (int i = 0; i < count; i++) {
        new_argv[i] = strdup(original_argv[i]);
    }

    // 4. Aseguramos que el último elemento sea NULL (indispensable para execvp)
    new_argv[count] = NULL;

    return new_argv;
}

// =============================================================================
// THREAD (alarm-thread y variantes)
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
// -----------------------------------------------------------------------------
//Funcion para el alarm thread, que funciona como timeout
// -----------------------------------------------------------------------------

void *alarm_thread_func(void *arg) {
    alarm_data_t *data = (alarm_data_t *)arg;    //Recargamos los datos

    sleep(data->seconds);       //Lanzamos el timeout con sleep

    killpg(data->pgid, SIGKILL);  //matamos
    killpg(data->pgid, SIGCONT);  //continuamos

    free(data);   //liberamos
    return NULL;
}

// -----------------------------------------------------------------------------
//Funcion para recorrer el directorio /proc/<pid>/stat y que imprima los zombie
// -----------------------------------------------------------------------------

void traverse_proc_zombie(int shell_pid) {
    DIR *d; 
    struct dirent *dir;
    char buff[2048];
    d = opendir("/proc");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            sprintf(buff, "/proc/%s/stat", dir->d_name); 
            FILE *fd = fopen(buff, "r");
            if (fd){
                long pid;     // pid
                long ppid;    // ppid
                char state;   // estado: R (runnable), S (sleeping), T(stopped), Z (zombie)

                // La siguiente línea lee pid, state y ppid de /proc/<pid>/stat
                fscanf(fd, "%ld %s %c %ld", &pid, buff, &state, &ppid);
                fclose(fd);
                if ( state == 'Z' && ppid == shell_pid ){
                    printf("%ld\n",pid);
                }
            }
        }
        closedir(d);
    }
}

// =============================================================================
// NUEVO MANEJADOR DE SEÑAL
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
// PROCESO TEMPORIZADOR (alarm-proc y variantes)
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


// -----------------------------------------------------------------------------
// manejador (SIGCHUP handler)
//Simplemente escribe "SIGHUP recibido.\n" en hup.txt
// ------------------------------------------------------------------------------


void manejadorHUP(int signal)
{

    FILE *fp;
    fp=fopen("hup.txt","a"); // abre un fichero en modo 'append'
    if (fp) { 
        fprintf(fp, "SIGHUP recibido.\n"); //escribe en el fichero
        fclose(fp);
    }

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

                if (current_job->state != RESPAWNABLE || WIFEXITED(wstatus) == EXIT_FAILURE){
                    printf("[%d] (%s) Terminated with status: %d\n",
                        current_job->pgid, current_job->command,
                        WEXITSTATUS(wstatus));
                    del_job(job_list, current_job);
                    free_job(current_job);
                    // No incrementamos i: el siguiente ocupa esta posición
                }else{

                    //Relanzo el proceso con los mismo argumentos

                    int new_pid = fork();
                    if ( new_pid == 0){

                        setpgid(0,0);
                        terminal_signals(SIG_DFL);
                        

                        // Sustituimos el código del hijo por el del programa a ejecutar
                        execvp(current_job->argv[0],current_job->argv);

                        perror(current_job->argv[0]); // Imprime el error [cite: 115]
                        exit(EXIT_FAILURE); // El hijo debe morir si falla execvp

                    }else{

                        setpgid(new_pid, new_pid);
                        current_job->pgid = new_pid;
                    }

                }
 
            } else if (WIFSIGNALED(wstatus)) {
                // El proceso terminó al recibir una señal (ej. SIGKILL)
                if ( current_job->state != RESPAWNABLE || WIFEXITED(wstatus) == EXIT_FAILURE ){
                    printf("[%d] (%s) Signaled by signal: %d\n",
                        current_job->pgid, current_job->command,
                        WTERMSIG(wstatus));
                    del_job(job_list, current_job);
                    free_job(current_job);
                    // No incrementamos i: el siguiente ocupa esta posición
                }else{

                    //Relanzo el proceso con los mismo argumentos

                    int new_pid = fork();
                    if ( new_pid == 0){

                        setpgid(0,0);
                        terminal_signals(SIG_DFL);
                        

                        // Sustituimos el código del hijo por el del programa a ejecutar
                        execvp(current_job->argv[0],current_job->argv);

                        perror(current_job->argv[0]); // Imprime el error [cite: 115]
                        exit(EXIT_FAILURE); // El hijo debe morir si falla execvp

                    }else{

                        setpgid(new_pid, new_pid);
                        current_job->pgid = new_pid;
                    }

                }
 
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
    int respawneable;
    int pid_fork, pid_wait;     // pid for created and waited process
    int wstatus;                // status returned by waitpid
    char *file_in, *file_out;   // for redirections

    int shell_pid = getpid(); // getpid() nos da el PID de nuestro propio programa
    int last_pid = 0;         // Al principio no hay un proceso anterior
    int retval = 0;           // Al principio el estado es 0

    job_list = new_list("jobs");

    
    terminal_signals(SIG_IGN);
    signal(SIGCHLD, manejador);
    signal(SIGHUP,manejadorHUP);

    while (1) {
        free_argv(argv);
        respawneable = 0;
        int ret = get_command("ShellSO > ", &argc, &argv);
        if (ret == -1) exit(EXIT_FAILURE);      // error in read(2)
        if (ret == 0) break;                    // finish loop if ^D (eof)
        if (argc == 0) continue;                // empty command: next iteration
        argc = parse_comments(argv);
        if (argc == 0) continue; // empty command after parsing comment #
        argc = parse_background(argv, &background);
        if (argc == 0) continue; // empty command after parsing background &
        subs_autovars(argv, shell_pid, last_pid, retval);

        //Añadimos mejora para que tambien haga append con >>
        int isAppend = 0;
        int i = 0;
        while( argv[i]!=NULL ){
            if (strcmp(argv[i], ">>")==0 ){
                isAppend = 1;
                argv[i][1] = '\0';         //Engañamos al parse_redirections para que saque el file in o file out
                break;
                }
            i++;
        }

        argc = parse_redirections(argv,  &file_in, &file_out);
        if (argc == 0) continue; // empty command after parsing redirections
        parse_escape(argv);

        //printf("file_in: %s\n",file_in);      //por si hay que hacer comprobaciones de las salidas
        //printf("file_out: %s\n",file_out);


        //Checkeamos si tiene el signo '+' de respawneable
        if (strcmp(argv[argc - 1], "+") == 0){
            background = 1;
            respawneable = 1;

            // Eliminamos el '+' de los argumentos para que no llegue al execvp
            free(argv[argc - 1]);
            argv[argc - 1] = NULL;
            argc--;
        }

        // ---------------------------------------------------------------------
        // Comandos internos: el shell los ejecuta directamente sin hacer fork
        // ---------------------------------------------------------------------

        //Comando interno mask
        //Sirve para enmascarar señales de cara al proceso hijo
        int mask_mode = 0;
        int signals[32]; //Incializamos esta variable para cada iteracion del bucle donde guardamos las señales
        int signal_count = 0; // variable donde contamos cuantas señales enmascara

         if (strcmp(argv[0], "mask") == 0) {

            int i = 0;

            while (argv[i] != NULL && strcmp(argv[i], "-c") != 0){
                i++;
            }
            
            if ( argv[i] == NULL || argv[i+1] == NULL){               //Aqui comprobamos que este -c y si esta que no sea el ultimo

                printf("mask: error de sintaxis\n");
                continue;

            }

            for ( int j = 1 ; j < i ; j++){       // Compruebo que todos los numeros son enteros
                if (atoi(argv[j])<=0){

                    printf("mask: error de sintaxis\n");
                    continue;

                } 
            }

             mask_mode = 1;

             for ( int j = 1 ; j < i ; j++){  //Guardamos las señales a bloquear
                signals[j-1] = atoi(argv[j]);
                signal_count++;
             }

             //Desplazamos el argv para que se ejecute el comando

             for ( int j = 0; j < argc - i; j++){

                argv[j] = argv[j+ i + 1];
             }

             argc -= i;  //Actualizamos el tamaño de argc
             argv[argc] = NULL; 


         }

         //Comando interno: alarm-thread
         //Inicializa un thread que sirve como timeout para matar el proceso lanzado si este no ha acabado en el tiempo indicado
         
         if (strcmp(argv[0], "alarm-thread") == 0) {


            if ( argc < 3 ){
                printf("No hay suficientes argumentos\n");
                continue;
            }

            if ( atoi(argv[1])<=0 ){
                printf("El tiempo de espera debe ser un entero positivo\n");
                continue;
            }

            char **args1 = &argv[2];      //Reestructuramos el array con los atributos

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

                // preparas los datos
                alarm_data_t *data = malloc(sizeof(alarm_data_t));
                data->seconds = atoi(argv[1]);  // los segundos del argumento
                data->pgid = pid_fork;          // el proceso a matar

                // configuras el thread como detached
                pthread_t tid;
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

                // creas el thread
                pthread_create(&tid, &attr, alarm_thread_func, data);

                // limpias los atributos
                pthread_attr_destroy(&attr);

                if ( background == 0 ){

                        // FOREGROUND: cedemos el terminal al hijo y esperamos

                        tcsetpgrp(STDIN_FILENO, pid_fork);   // ceder terminal al child

                        waitpid(pid_fork, &wstatus, WUNTRACED);

                        tcsetpgrp(STDIN_FILENO, getpid());   // recuperar terminal
                        

                        if (WIFEXITED(wstatus)){
                            
                            // Terminó normalmente
                            printf("[%d] (%s) Terminated with status: %d\n", pid_fork, args1[0], WEXITSTATUS(wstatus));

                        }else if (WIFSIGNALED(wstatus)){

                            // Terminó al recibir una señal (ej. CTRL+C → SIGINT)
                            printf("[%d] (%s) Signaled by signal: %d\n", pid_fork, args1[0], WTERMSIG(wstatus));

                        }else if (WIFSTOPPED(wstatus)) {

                            // Se suspendió (ej. CTRL+Z → SIGTSTP)
                            mask_signal(SIGCHLD, SIG_BLOCK);
                            add_job(job_list, new_job(pid_fork, args1[0], STOPPED));
                            mask_signal(SIGCHLD, SIG_UNBLOCK);
                            printf("[%d] (%s) Stopped by signal: %d\n", pid_fork, args1[0], WSTOPSIG(wstatus));
                        }
                
                }else{

                        // BACKGROUND: no cedemos el terminal, el shell sigue activo
                        // Añadimos el job a la lista para poder gestionarlo después
                        if(respawneable != 1){
                            mask_signal(SIGCHLD, SIG_BLOCK);
                            add_job(job_list, new_job(pid_fork,args1[0],BACKGROUND));
                            mask_signal(SIGCHLD, SIG_UNBLOCK);
                            printf("[%d] (%s) Running in Background\n", pid_fork, args1[0]);
                        }else{

                            mask_signal(SIGCHLD, SIG_BLOCK);

                            // 1. Creamos el job especificando su estado RESPAWNABLE
                            job *new_j = new_job(pid_fork, args1[0], RESPAWNABLE);
                
                            // 2. Le asignamos la copia profunda de los argumentos
                            new_j->argv = copy_argv(args1); 
                
                            // 3. Añadimos el job modificado a la lista
                            add_job(job_list, new_j);

                            mask_signal(SIGCHLD, SIG_UNBLOCK);
                            printf("[%d] (%s) Running in Respawneable\n", pid_fork, args1[0]);


                        }
                }

            }
            continue;

            /* Proceso de creación del thread:
            1. Prepara el formulario      → pthread_attr_init
            2. Rellena el formulario      → pthread_attr_setdetachstate        (Puede ser joined ( necesita que alguien le haga pthread_join) o detached (se acaba solo))
            3. Lanza el thread            → pthread_create
            4. Tira el formulario         → pthread_attr_destroy
            */
         }

        

        //Comando interno: bg team
        //Crea varios varios procesos iguales en bakcground

        if (strcmp(argv[0], "bgteam") == 0) {

            if ( argc < 3 ){

                printf("El comando bgteam requiere dos argumentos\n");
                continue;

            }

            int numberOfCmd = atoi(argv[1]);  // Guardo el numero de veces que queremos lanzar el cmd

            if (numberOfCmd < 1 ){  //Si es menor que 1 salta
                continue;
            }


            for ( int j = 0; j < argc - 2; j++){   //Desplazamos argv para que empiece en el comando
                argv[j] = argv[j+ 2];
            }
            argc-=2;                            //Actualizamos parametros para evitar confusiones de execvp
            argv[argc] = NULL;
            
            if ( strcmp(argv[argc -1 ],"&") == 0 ){    // Si lleva la muletilla & la quitamos

                argv[argc-1] = NULL;
            
            }


            for(int i = 0; i < numberOfCmd ; i++){

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
                    
                    if ( mask_mode == 1 ){
                        for ( int i = 0; i < signal_count; i++){

                            mask_signal(signals[i], SIG_BLOCK);

                        }
                    }

                    // Sustituimos el código del hijo por el del programa a ejecutar
                    execvp(argv[0],argv);

                    perror(argv[0]); // Imprime el error [cite: 115]
                    exit(EXIT_FAILURE); // El hijo debe morir si falla execvp

                }else{

                    // BACKGROUND: no cedemos el terminal, el shell sigue activo
                    // Añadimos el job a la lista para poder gestionarlo después
                    mask_signal(SIGCHLD, SIG_BLOCK);
                    add_job(job_list, new_job(pid_fork,argv[0],BACKGROUND));
                    mask_signal(SIGCHLD, SIG_UNBLOCK);
                    printf("[%d] (%s) Running in Background\n", pid_fork, argv[0]);

                }
            }

            continue;
        }
 
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

        //Comando interno: currjob
        //Imprime el job actual

        if ( strcmp(argv[0], "currjob") == 0){

            if ( job_list->count > 0){

                job *current_job = get_job_bypos(job_list,1);

                printf( "Trabajo actual: PID=%d command=%s\n", current_job->pgid, current_job->command);
            
            }else{
                printf("No hay trabajo actual\n");
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

        //Comando interno: deljob
        //Elimina el job actual (primero de la lista), si esta en background no suspendido

        if ( strcmp(argv[0], "deljob") == 0){

            if ( job_list->count == 0 ){               //Comprobamos si la lista esta vacia
                printf("No hay trabajo actual\n");
                continue;
            }

            job *j = get_job_bypos(job_list,1);           //Cogemos el actual

            if ( j->state == STOPPED ){       //Comprobamos si esta en stopped
                printf("No se permiten borrar trabajos en segundo plano suspendidos\n");
                continue;
            }

            if (j->state == BACKGROUND){         //Comprobamos que sea background

                printf("Borrando trabajo actual de la lista de jobs: PID=%d command=%s\n", j->pgid, j->command);
                mask_signal(SIGCHLD, SIG_BLOCK);  //Bloqueamos señales
                del_job(job_list,j);       //Lo quitamos de la lista
                mask_signal(SIGCHLD, SIG_UNBLOCK);        //Devolvemos señales
                continue;
            }

        }

        //Comando interno: zjobs
        //Busca los procesos en estado zombie dentro del Shell y lista sus pids

        if ( strcmp(argv[0], "zjobs") == 0){
            traverse_proc_zombie(shell_pid);
            continue;
        }



        // Comando interno: lanzabg
        // Actua de forma identica a colocar un & al final del comando

        if ( strcmp(argv[0], "lanzabg") == 0){

            if (argv[1] == NULL) {
                printf("lanzabg: falta el comando\n");
                continue;
            }
            char **args1 = &argv[1];      //Reestructuramos el array con los atributos

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

        //Comando interno: fico
        //Cuenta los ficheros que empiezan por algo o por nada en proc

        if (strcmp(argv[0], "fico") == 0) {

            // construimos el argv para el script
            // argv[0] → "./cuentafich.sh"
            // argv[1] → el prefijo si lo hay (argv[1] del comando fico)
            char *script_argv[3];
            script_argv[0] = "./cuentafich.sh";
            script_argv[1] = argv[1];  // puede ser NULL si no hay argumento
            script_argv[2] = NULL;

            pid_fork = fork();

            if (pid_fork < 0) {
                perror("Error en fork");

            } else if (pid_fork == 0) {
                setpgid(0, 0);
                terminal_signals(SIG_DFL);
                execvp(script_argv[0], script_argv);
                perror(script_argv[0]);
                exit(EXIT_FAILURE);

            } else {
                setpgid(pid_fork, pid_fork);

                if (background == 0) {
                    tcsetpgrp(STDIN_FILENO, pid_fork);
                    waitpid(pid_fork, &wstatus, WUNTRACED);
                    tcsetpgrp(STDIN_FILENO, getpid());

                    if (WIFEXITED(wstatus)) {
                        printf("[%d] (fico) Terminated with status: %d\n", pid_fork, WEXITSTATUS(wstatus));
                    } else if (WIFSIGNALED(wstatus)) {
                        printf("[%d] (fico) Signaled by signal: %d\n", pid_fork, WTERMSIG(wstatus));
                    } else if (WIFSTOPPED(wstatus)) {
                        mask_signal(SIGCHLD, SIG_BLOCK);
                        add_job(job_list, new_job(pid_fork, "fico", STOPPED));
                        mask_signal(SIGCHLD, SIG_UNBLOCK);
                        printf("[%d] (fico) Stopped by signal: %d\n", pid_fork, WSTOPSIG(wstatus));
                    }
                } else {
                    mask_signal(SIGCHLD, SIG_BLOCK);
                    add_job(job_list, new_job(pid_fork, "fico", BACKGROUND));
                    mask_signal(SIGCHLD, SIG_UNBLOCK);
                    printf("[%d] (fico) Running in Background\n", pid_fork);
                }
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
                int fd;
                
                if (isAppend == 1) {
                    // isAppend es verdadero -> usamos O_APPEND (>>)
                    fd = open(file_out, O_WRONLY | O_CREAT | O_APPEND, 0644);
                } else {
                    // isAppend es falso -> usamos O_TRUNC (>)
                    fd = open(file_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                }
                
                if (fd == -1) { perror(file_out); exit(EXIT_FAILURE); }
                dup2(fd, STDOUT_FILENO);   // stdout ahora escribe en el fichero
                close(fd);
            }
                
            
            if ( mask_mode == 1 ){
                for ( int i = 0; i < signal_count; i++){

                    mask_signal(signals[i], SIG_BLOCK);

                }
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
                if(respawneable != 1){
                    mask_signal(SIGCHLD, SIG_BLOCK);
                    add_job(job_list, new_job(pid_fork,argv[0],BACKGROUND));
                    mask_signal(SIGCHLD, SIG_UNBLOCK);
                    printf("[%d] (%s) Running in Background\n", pid_fork, argv[0]);
                }else{

                    mask_signal(SIGCHLD, SIG_BLOCK);

                    // 1. Creamos el job especificando su estado RESPAWNABLE
                    job *new_j = new_job(pid_fork, argv[0], RESPAWNABLE);
        
                    // 2. Le asignamos la copia profunda de los argumentos
                    new_j->argv = copy_argv(argv); 
        
                    // 3. Añadimos el job modificado a la lista
                    add_job(job_list, new_j);

                    mask_signal(SIGCHLD, SIG_UNBLOCK);
                    printf("[%d] (%s) Running in Respawneable\n", pid_fork, argv[0]);


                }
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


// =============================================================================
// killall - matar todos los jobs de la lista
// =============================================================================


/*
if (strcmp(argv[0], "killall") == 0) {
    mask_signal(SIGCHLD, SIG_BLOCK);
    while (job_list->count > 0) {
        job *j = get_job_bypos(job_list, 1);
        killpg(j->pgid, SIGKILL);
        del_job(job_list, j);
        free_job(j);
    }
    mask_signal(SIGCHLD, SIG_UNBLOCK);
    continue;
}
*/


// =============================================================================
// stopall - suspender todos los jobs en background
// =============================================================================


/*
if (strcmp(argv[0], "stopall") == 0) {
    mask_signal(SIGCHLD, SIG_BLOCK);
    for (int i = 1; i <= job_list->count; i++) {
        job *j = get_job_bypos(job_list, i);
        if (j->state == BACKGROUND) {
            killpg(j->pgid, SIGSTOP);
            j->state = STOPPED;
            printf("[%d] (%s) Stopped\n", j->pgid, j->command);
        }
    }
    mask_signal(SIGCHLD, SIG_UNBLOCK);
    continue;
}
*/


// =============================================================================
// contjobs - contar jobs por estado
// =============================================================================


/*
if (strcmp(argv[0], "contjobs") == 0) {
    int bg = 0, stopped = 0;
    mask_signal(SIGCHLD, SIG_BLOCK);
    for (int i = 1; i <= job_list->count; i++) {
        job *j = get_job_bypos(job_list, i);
        if (j->state == BACKGROUND) bg++;
        if (j->state == STOPPED)    stopped++;
    }
    mask_signal(SIGCHLD, SIG_UNBLOCK);
    printf("Background: %d, Stopped: %d\n", bg, stopped);
    continue;
}
*/


// =============================================================================
// infojob - info detallada de un job por posición
// =============================================================================


/*
if (strcmp(argv[0], "infojob") == 0) {
    if (argv[1] == NULL) {
        printf("infojob: falta el número de job\n");
        continue;
    }
    mask_signal(SIGCHLD, SIG_BLOCK);
    job *j = get_job_bypos(job_list, atoi(argv[1]));
    if (j == NULL) {
        printf("infojob: no existe el job %s\n", argv[1]);
    } else {
        printf("PID=%d command=%s state=%s\n",
               j->pgid, j->command, state_strings[j->state]);
    }
    mask_signal(SIGCHLD, SIG_UNBLOCK);
    continue;
}
*/


// =============================================================================
// bgall - reanudar todos los jobs suspendidos en background
// =============================================================================


/*
if (strcmp(argv[0], "bgall") == 0) {
    mask_signal(SIGCHLD, SIG_BLOCK);
    for (int i = 1; i <= job_list->count; i++) {
        job *j = get_job_bypos(job_list, i);
        if (j->state == STOPPED) {
            j->state = BACKGROUND;
            killpg(j->pgid, SIGCONT);
            printf("[%d] (%s) Running in BACKGROUND\n", j->pgid, j->command);
        }
    }
    mask_signal(SIGCHLD, SIG_UNBLOCK);
    continue;
}
*/


// =============================================================================
// sendjob - mandar una señal a un job por posición
// =============================================================================


/*
if (strcmp(argv[0], "sendjob") == 0) {
    if (argv[1] == NULL || argv[2] == NULL) {
        printf("sendjob: uso: sendjob <posicion> <señal>\n");
        continue;
    }
    int pos = atoi(argv[1]);
    int sig = atoi(argv[2]);
    mask_signal(SIGCHLD, SIG_BLOCK);
    job *j = get_job_bypos(job_list, pos);
    if (j == NULL) {
        printf("sendjob: no existe el job %d\n", pos);
    } else {
        killpg(j->pgid, sig);
        printf("[%d] (%s) Signal %d sent\n", j->pgid, j->command, sig);
    }
    mask_signal(SIGCHLD, SIG_UNBLOCK);
    continue;
}
*/


// =============================================================================
// myjobs - versión personalizada de jobs con formato diferente
// =============================================================================


/*
if (strcmp(argv[0], "myjobs") == 0) {
    mask_signal(SIGCHLD, SIG_BLOCK);
    int found = 0;
    for (int i = 1; i <= job_list->count; i++) {
        job *j = get_job_bypos(job_list, i);
        if (j->state == BACKGROUND) {
            printf("[%d] %s\n", j->pgid, j->command);
            found = 1;
        }
    }
    if (!found) printf("No hay jobs en background\n");
    mask_signal(SIGCHLD, SIG_UNBLOCK);
    continue;
}
*/


// =============================================================================
// exec - ejecutar un script interno del shell
// =============================================================================


/*
if (strcmp(argv[0], "exec") == 0) {
    if (argv[1] == NULL) {
        printf("exec: falta el fichero\n");
        continue;
    }
    // Simplemente ejecutarlo como comando externo con sh
    char *script_argv[3];
    script_argv[0] = "sh";
    script_argv[1] = argv[1];
    script_argv[2] = NULL;
    pid_fork = fork();
    if (pid_fork == 0) {
        setpgid(0, 0);
        terminal_signals(SIG_DFL);
        execvp(script_argv[0], script_argv);
        perror(script_argv[0]);
        exit(EXIT_FAILURE);
    } else {
        setpgid(pid_fork, pid_fork);
        tcsetpgrp(STDIN_FILENO, pid_fork);
        waitpid(pid_fork, &wstatus, WUNTRACED);
        tcsetpgrp(STDIN_FILENO, getpid());
        if (WIFEXITED(wstatus))
            printf("[%d] (exec) Terminated with status: %d\n",
                   pid_fork, WEXITSTATUS(wstatus));
    }
    continue;
}
*/


// =============================================================================
// nohup - lanzar comando inmune a SIGHUP
// =============================================================================


/*
if (strcmp(argv[0], "nohup") == 0) {
    if (argv[1] == NULL) {
        printf("nohup: falta el comando\n");
        continue;
    }
    char **args = &argv[1];
    pid_fork = fork();
    if (pid_fork == 0) {
        setpgid(0, 0);
        terminal_signals(SIG_DFL);
        signal(SIGHUP, SIG_IGN);   // ignorar SIGHUP
        execvp(args[0], args);
        perror(args[0]);
        exit(EXIT_FAILURE);
    } else {
        setpgid(pid_fork, pid_fork);
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
// repeat N cmd - lanzar N veces el mismo comando en background
// =============================================================================


/*
if (strcmp(argv[0], "repeat") == 0) {
    if (argc < 3 || atoi(argv[1]) <= 0) {
        printf("repeat: uso: repeat <N> <comando> [args...]\n");
        continue;
    }
    int N = atoi(argv[1]);
    char **args = &argv[2];
    for (int k = 0; k < N; k++) {
        pid_fork = fork();
        if (pid_fork == 0) {
            setpgid(0, 0);
            terminal_signals(SIG_DFL);
            execvp(args[0], args);
            perror(args[0]);
            exit(EXIT_FAILURE);
        } else {
            setpgid(pid_fork, pid_fork);
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
// alarm <seconds>
// =============================================================================

/*
if (strcmp(argv[0], "alarm") == 0) {
    if (argc < 3) { printf("alarm: faltan argumentos\n"); continue; }
    
    // construir el mensaje uniendo todos los argumentos desde argv[2]
    char mensaje[1024] = "";
    int segundos = atoi(argv[1]);
    for (int i = 2; argv[i] != NULL; i++) {
        if (i > 2) strcat(mensaje, " ");
        strcat(mensaje, argv[i]);
    }
    
    // usar un thread para no bloquear el shell
    // estructura de datos:
    // typedef struct { int seconds; char mensaje[1024]; } alarm_data_t;
    
    alarm_data_t *data = malloc(sizeof(alarm_data_t));
    data->seconds = segundos;
    strcpy(data->mensaje, mensaje);
    
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &attr, alarm_thread_func, data);
    pthread_attr_destroy(&attr);
    
    // NO hay continue aquí todavía, sigue al fork... espera,
    // en este caso NO queremos fork. El thread es suficiente.
    continue;
}

// función del thread:
void *alarm_thread_func(void *arg) {
    alarm_data_t *data = (alarm_data_t *)arg;
    sleep(data->seconds);
    printf("\n%s\n", data->mensaje);
    fflush(stdout);
    free(data);
    return NULL;
}
*/

// =============================================================================
// %<job>
// =============================================================================

/*
// justo antes de los comandos internos, después del parse:
if (argv[0][0] == '%') {
    // extraer el número después del %
    char *num_str = &argv[0][1];  // apunta al carácter después del %
    
    if (strlen(num_str) == 0) {
        printf("%%: falta el número de job\n");
        continue;
    }
    
    // verificar que es un número
    char *endptr;
    long pos = strtol(num_str, &endptr, 10);
    if (*endptr != '\0' || pos <= 0) {
        printf("%%: número de job inválido\n");
        continue;
    }
    
    // a partir de aquí es exactamente igual que fg con argumento
    job *working_job = get_job_bypos(job_list, pos);
    
    if (working_job == NULL) {
        printf("%%: no existe el job %ld\n", pos);
        continue;
    }
    
    // ... mismo código que fg ...
    printf("[%d] (%s) Running in FOREGROUND\n", working_job->pgid, working_job->command);
    mask_signal(SIGCHLD, SIG_BLOCK);
    del_job(job_list, working_job);
    mask_signal(SIGCHLD, SIG_UNBLOCK);
    tcsetpgrp(STDIN_FILENO, working_job->pgid);
    working_job->state = FOREGROUND;
    killpg(working_job->pgid, SIGCONT);
    waitpid(working_job->pgid, &wstatus, WUNTRACED);
    tcsetpgrp(STDIN_FILENO, getpid());
    // ... resto igual que fg ...
    
    continue;
}

*/

// =============================================================================
// Contador de suspensiones
// =============================================================================
/*
Paso 1: añadir campo a la estructura job en job_control.h:
typedef struct {
    pid_t pgid;
    char *command;
    enum job_state state;
    char **argv;
    int stop_count;   // ← añadir esto
} job;

Paso 2: inicializar a 0 en new_job en job_control.c:
aux->stop_count = 0;

Paso 3: incrementar cada vez que se suspende. Hay dos sitios donde ocurre una suspensión:
En el bucle principal (foreground suspendido con CTRL+Z):

else if (WIFSTOPPED(wstatus)) {
    job *j = new_job(pid_fork, argv[0], STOPPED);
    j->stop_count = 1;   // primera suspensión
    mask_signal(SIGCHLD, SIG_BLOCK);
    add_job(job_list, j);
    mask_signal(SIGCHLD, SIG_UNBLOCK);
    printf("[%d] (%s) Stopped by signal: %d\n", ...);
}

En el manejador (background suspendido):
else if (WIFSTOPPED(wstatus)) {
    current_job->state = STOPPED;
    current_job->stop_count++;   // ← incrementar
    i++;
}

Y en fg cuando se vuelve a suspender:
else if (WIFSTOPPED(wstatus)) {
    working_job->state = STOPPED;
    working_job->stop_count++;   // ← incrementar
    ...
}
Paso 4: imprimir al terminar en foreground:
if (WIFEXITED(wstatus)) {
    printf("[%d] (%s) Terminated with status: %d (suspendido %d veces)\n",
           pid_fork, argv[0], WEXITSTATUS(wstatus), ???);
}
           
El problema aquí es que cuando el proceso termina desde foreground directamente, no está en la lista. Si nunca se suspendió, stop_count sería 0. Tienes que guardarlo en algún sitio antes de perderlo.
La solución más simple es tener una variable local int stop_count_fg = 0 que incrementas cada vez que el proceso foreground se suspende y vuelves a hacer fg:
cint stop_count_fg = 0;
// resetear al inicio del bucle

// cuando se suspende en foreground:
stop_count_fg++;  // o usar working_job->stop_count si viene de fg

// cuando termina:
printf("[%d] (%s) Terminated with status: %d (suspendido %d veces)\n",
       pid_fork, argv[0], WEXITSTATUS(wstatus), stop_count_fg);
*/
