// -----------------------------------------------------------------------------
// UNIX Shell Project
// job control
//
// Sistemas Operativos
// Grados I. Informatica, Computadores & Software
// Dept. Arquitectura de Computadores - UMA
//
// Adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

// Nombre: Jorge Barrios Lara
// Asignatura: Sistemas Operativos 2025/2026
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

/*
# =============================================================================
# BUCLES
# =============================================================================

# for con lista de valores:
for i in 1 2 3 4 5; do
    echo $i
done

# for con rango (seq):
for i in $(seq 1 10); do
    echo $i
done

# for recorriendo ficheros del directorio actual:
for f in *; do
    if [ -f "$f" ]; then
        echo "Fichero: $f"
    fi
done

# for recorriendo argumentos del script:
for arg in "$@"; do
    echo "Argumento: $arg"
done

# while:
N=0
while [ $N -lt 10 ]; do
    N=$((N+1))
    echo $N
done

# until (hasta que la condición sea verdadera):
until [ $N -eq 0 ]; do
    N=$((N-1))
done

# break y continue:
for i in 1 2 3 4 5; do
    if [ $i -eq 2 ]; then continue; fi   # saltar iteración
    if [ $i -eq 4 ]; then break; fi      # salir del bucle
    echo $i
done


# =============================================================================
# CAPTURAR SALIDA DE COMANDOS
# =============================================================================
N=$(ls | wc -l)              # contar ficheros
FECHA=$(date)                # capturar fecha
LINEAS=$(wc -l < fichero)    # contar líneas de un fichero
RESULTADO=$(comando)         # guardar cualquier salida


# =============================================================================
# PIPES Y FILTROS ÚTILES
# =============================================================================

# Contar ficheros (no directorios) en directorio actual:
N=$(ls -pA | grep -v "/" | wc -l)

# Contar ficheros con prefijo:
N=$(ls -pA | grep -v "/" | grep "^PREFIJO" | wc -l)

# Contar ficheros con extensión:
N=$(ls -pA | grep -v "/" | grep "\.ext$" | wc -l)

# Contar directorios:
N=$(ls -pA | grep "/" | wc -l)

# Buscar texto en ficheros:
N=$(grep -rl "texto" . 2>/dev/null | wc -l)

# Contar procesos de un usuario:
N=$(ps -u $USER --no-headers 2>/dev/null | wc -l)

# Notas sobre ls:
# ls -p  → añade / al final de directorios
# ls -A  → incluye ficheros ocultos (excepto . y ..)
# ls -1  → un fichero por línea
# ls -pA → combinación: incluye ocultos y marca directorios

# =============================================================================
# FUNCIONES
# =============================================================================
mi_funcion() {
    local resultado=$1    # local: variable local a la función
    echo "Argumento: $resultado"
    return 0              # valor de retorno (0-255)
}

mi_funcion "hola"
echo "Retornó: $?"


# =============================================================================
# OPERACIONES ÚTILES
# =============================================================================
STR="hola mundo"
echo ${#STR}              # longitud: 10
echo ${STR:0:4}           # subcadena: "hola"
echo ${STR/hola/adios}    # reemplazar primera ocurrencia
echo ${STR^^}             # HOLA MUNDO (mayúsculas)
echo ${STR,,}             # hola mundo (minúsculas)

# Valor por defecto si variable no existe:
PREFIJO=${1:-""}          # si $1 no está, usar ""
USER=${1:-$(whoami)}      # si $1 no está, usar el usuario actual


# =============================================================================
# ARRAYS
# =============================================================================
arr=(1 2 3 4 5)           # declarar array
echo ${arr[0]}            # primer elemento: 1
echo ${arr[-1]}           # último elemento: 5
echo ${arr[@]}            # todos los elementos
echo ${#arr[@]}           # número de elementos: 5
arr+=(6)                  # añadir elemento al final

# Recorrer array:
for elem in "${arr[@]}"; do
    echo $elem
done


# =============================================================================
# ENTRADA/SALIDA Y REDIRECCIONES
# =============================================================================
echo "texto"              # imprimir con salto de línea
printf "texto\n"          # más control del formato
read VAR                  # leer línea del teclado
read -p "Nombre: " VAR    # leer con prompt

# Redirecciones:
comando > fichero         # sobreescribir
comando >> fichero        # añadir al final
comando < fichero         # leer desde fichero
comando 2> errores.txt    # redirigir stderr
comando &> todo.txt       # redirigir stdout y stderr
comando 2>/dev/null       # descartar errores

# =============================================================================
# MISCELÁNEA ÚTIL
# =============================================================================
sleep 5                   # esperar 5 segundos (para el evaluador)
exit 0                    # salir con éxito
exit 1                    # salir con error
chmod +x script.sh        # dar permisos de ejecución

# Comprobar si un proceso existe:
if kill -0 $PID 2>/dev/null; then
    echo "El proceso $PID existe"
fi

# Comprobar si un comando existe:
if command -v gcc &>/dev/null; then
    echo "gcc existe"
fi

# Ejecutar comando y capturar código de retorno:
ls /no/existe 2>/dev/null
if [ $? -eq 0 ]; then
    echo "OK"
else
    echo "Error"
fi

# Equivalente más corto:
if ls /no/existe 2>/dev/null; then
    echo "OK"
else
    echo "Error"
fi

*/


