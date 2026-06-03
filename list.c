// -----------------------------------------------------------------------------
// UNIX Shell Project
// Naive list implementation
// -----------------------------------------------------------------------------
// Nombre: Jorge Barrios Lara
// Asignatura: Sistemas Operativos 2025/2026

#include "list.h"

#include <stdlib.h>  // for malloc/free
#include <string.h>  // for strdup
#include <stdio.h> //for debug

// -----------------------------------------------------------------------------
// --------Functions for LIST management----------------------------------------
// -----------------------------------------------------------------------------
// creates a new list with a name
list_head_t* new_list(char *name)
{
    list_head_t *lp = malloc(sizeof(list_head_t));
    lp->name = strdup(name);
    lp->count = 0;
    lp->first = NULL;
    return lp;
}

// -----------------------------------------------------------------------------
// insert an item at the head of the list
// returns 1 on success and 0 in case of error
int insert_item(list_head_t *list, void *item)
{
    list_item_t *aux = list->first;
    list_item_t *elem = malloc(sizeof(list_item_t));
    if (!elem) return 0; 
    elem->data = item;
    elem->next = aux;
    list->first = elem;
    list->count++;
    return 1;
}
// -----------------------------------------------------------------------------
// elimina el elemento indicado de la lista
// devuelve 0 si no estaba en la lista
int remove_item(list_head_t *list, void *item)
{
    list_item_t *aux = list->first;
    list_item_t **prev = &list->first;
    while ((aux != NULL) && (aux->data != item)) {
        prev = &aux->next;
        aux = aux->next;
    }
    if (!aux) return 0;
    list->count--;
    *prev = aux->next;
    free(aux);
    return 1;
}
// -----------------------------------------------------------------------------
// devuelve n>0 (true) siendo n la posicion donde se encuentra el elemento
// devuelve 0   (false) si no esta en la lista
unsigned int find_item(list_head_t *list, void* item)
{
    unsigned int n;
    list_item_t *aux = list->first;
    for (n = 1; aux; n++, aux = aux->next) {
        if (aux->data == item) return n;    // found in position n
    }
    return 0;   // not found
}

// -----------------------------------------------------------------------------
// retorna el n-esimo elemento de la lista, si no existe retorna NULL
void * get_item_bypos(list_head_t *list, unsigned int n)
{
    list_item_t *aux = list->first;
    if ((n < 1) || (n > list->count)) return NULL;
    while (--n && aux) aux = aux->next;
    return (aux? aux->data: NULL);
}

// Search by content
// busca y devuelve el primer elemento de la lista que cumpla la condicion
// evaluada por la funcion check (que evalua cada elemento)
// retornando 1 si cumple la condicion y 0 si no la cumple
// devuelve NULL si ningun elemento de la lista cumple la condicion
void * get_item_byfunc(list_head_t *list, int(*check)(void *))
{
    list_item_t *aux = list->first;
    while (aux && !check(aux->data)) aux = aux->next;
    return (aux? aux->data: NULL);
}

// -----------------------------------------------------------------------------
// recorre la lista y le aplica la funcion func a cada elemento
void traverse_list(list_head_t *list, void (*func)(void *, unsigned int))
{
    unsigned int n;
    list_item_t *aux = list->first;
    for (n = 1; aux; n++, aux = aux->next) {
        func(aux->data, n);
    }
}

// -----------------------------------------------------------------------------

/*
# =============================================================================
cuentafich.sh - contar ficheros con prefijo (ya implementado)
# =============================================================================
# Uso: ./cuentafich.sh [prefijo]

cat << 'EOF' > cuentafich.sh
#!/bin/bash
if [ $# -eq 1 ]; then
    PREFIJO=$1
else
    PREFIJO=""
fi
N=$(ls -pA | grep -v "/" | grep "^$PREFIJO" | wc -l)
echo "Número de ficheros encontrados: $N"
sleep 5
if [ $N -gt 0 ]; then exit 0; else exit 1; fi
EOF


# =============================================================================
# cuentadir.sh - contar directorios con prefijo
# =============================================================================
# Uso: ./cuentadir.sh [prefijo]
# Similar a cuentafich pero cuenta directorios (terminan en /)

cat << 'EOF' > cuentadir.sh
#!/bin/bash
if [ $# -eq 1 ]; then
    PREFIJO=$1
else
    PREFIJO=""
fi
# ls -p pone / al final de directorios, grep "/" los selecciona
N=$(ls -pA | grep "/" | grep "^$PREFIJO" | wc -l)
echo "Número de directorios encontrados: $N"
sleep 5
if [ $N -gt 0 ]; then exit 0; else exit 1; fi
EOF


# =============================================================================
# cuentaext.sh - contar ficheros por extensión
# =============================================================================
# Uso: ./cuentaext.sh <extension>
# Ejemplo: ./cuentaext.sh txt  → cuenta ficheros .txt

cat << 'EOF' > cuentaext.sh
#!/bin/bash
if [ $# -eq 0 ]; then
    echo "Uso: cuentaext.sh <extension>"
    exit 1
fi
EXT=$1
N=$(ls -pA | grep -v "/" | grep "\.${EXT}$" | wc -l)
echo "Ficheros .${EXT} encontrados: $N"
sleep 5
if [ $N -gt 0 ]; then exit 0; else exit 1; fi
EOF


# =============================================================================
# cuentalineas.sh - contar líneas totales de ficheros con prefijo
# =============================================================================
# Uso: ./cuentalineas.sh [prefijo]

cat << 'EOF' > cuentalineas.sh
#!/bin/bash
PREFIJO=${1:-""}
TOTAL=0
for f in $(ls -pA | grep -v "/" | grep "^$PREFIJO"); do
    if [ -f "$f" ]; then
        N=$(wc -l < "$f")
        TOTAL=$((TOTAL + N))
    fi
done
echo "Total de líneas: $TOTAL"
sleep 5
if [ $TOTAL -gt 0 ]; then exit 0; else exit 1; fi
EOF


# =============================================================================
# busca.sh - buscar texto en ficheros del directorio actual
# =============================================================================
# Uso: ./busca.sh <texto>

cat << 'EOF' > busca.sh
#!/bin/bash
if [ $# -eq 0 ]; then
    echo "Uso: busca.sh <texto>"
    exit 1
fi
TEXTO=$1
N=$(grep -rl "$TEXTO" . 2>/dev/null | wc -l)
echo "Ficheros que contienen '$TEXTO': $N"
sleep 5
if [ $N -gt 0 ]; then exit 0; else exit 1; fi
EOF


# =============================================================================
# tamfich.sh - listar ficheros mayores de N bytes
# =============================================================================
# Uso: ./tamfich.sh <tamanio_minimo_bytes>

cat << 'EOF' > tamfich.sh
#!/bin/bash
MIN=${1:-0}
N=0
for f in $(ls -pA | grep -v "/"); do
    if [ -f "$f" ]; then
        TAM=$(wc -c < "$f")
        if [ $TAM -gt $MIN ]; then
            echo "$f: $TAM bytes"
            N=$((N + 1))
        fi
    fi
done
echo "Total ficheros mayores de $MIN bytes: $N"
sleep 5
if [ $N -gt 0 ]; then exit 0; else exit 1; fi
EOF


# =============================================================================
# cuentaproc.sh - contar procesos de un usuario
# =============================================================================
# Uso: ./cuentaproc.sh [usuario]

cat << 'EOF' > cuentaproc.sh
#!/bin/bash
USER=${1:-$(whoami)}
N=$(ps -u "$USER" --no-headers 2>/dev/null | wc -l)
echo "Procesos de $USER: $N"
sleep 5
if [ $N -gt 0 ]; then exit 0; else exit 1; fi
EOF


# =============================================================================
# checkpid.sh - comprobar si un proceso existe
# =============================================================================
# Uso: ./checkpid.sh <pid>
# Retorna 0 si existe, 1 si no

cat << 'EOF' > checkpid.sh
#!/bin/bash
if [ $# -eq 0 ]; then
    echo "Uso: checkpid.sh <pid>"
    exit 1
fi
PID=$1
if kill -0 $PID 2>/dev/null; then
    echo "El proceso $PID existe"
    exit 0
else
    echo "El proceso $PID no existe"
    exit 1
fi
EOF


# =============================================================================
# NOTAS IMPORTANTES PARA SCRIPTS BASH EN EL EXAMEN
# =============================================================================
# 
# CONTAR FICHEROS (no directorios):
#   ls -pA | grep -v "/"             → lista sin directorios
#   ls -pA | grep -v "/" | wc -l     → cuenta
#
# CONTAR DIRECTORIOS:
#   ls -pA | grep "/"                → solo directorios (tienen / al final)
#
# FILTRAR POR PREFIJO:
#   grep "^PREFIJO"                  → líneas que empiezan por PREFIJO
#   Si PREFIJO está vacío, "^" coincide con todo
#
# FILTRAR POR EXTENSIÓN:
#   grep "\.ext$"                    → líneas que terminan en .ext
#
# VARIABLES ÚTILES:
#   $#    → número de argumentos
#   $1    → primer argumento
#   $?    → código de retorno del último comando
#   $(comando)  → captura la salida de un comando
#
# COMPARACIONES EN IF:
#   -eq   igual
#   -ne   distinto
#   -gt   mayor que
#   -lt   menor que
#   -ge   mayor o igual
#   -le   menor o igual
#   -f    es un fichero regular
#   -d    es un directorio
#   -z    cadena vacía
#   -n    cadena no vacía
#
# ARITMÉTICA:
#   N=$((N + 1))    → incrementar
#   N=$((A * B))    → multiplicar
#
# SIEMPRE AÑADIR AL FINAL:
#   sleep 5          → para que el evaluador pueda monitorizar
#   exit 0 / exit 1  → código de retorno significativo
#   chmod +x script.sh → dar permisos de ejecución

*/

