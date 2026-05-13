// -----------------------------------------------------------------------------
// UNIX Shell Project
// Naive list implementation
// -----------------------------------------------------------------------------

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
