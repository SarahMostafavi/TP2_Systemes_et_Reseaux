#include <stdio.h>
#include "malloc_3is.h"
#include "free_3is.h"

/* Tête de la liste chaînée des blocs libres, triée par adresse croissante. */
static HEADER *free_list = NULL;


void free_3is(void *ptr)
{
    /* free(NULL) ne fait rien */
    if (ptr == NULL)
        return;

    /* ptr pointe sur le memory bloc : le header est juste avant */
    HEADER *header = (HEADER *)ptr - 1;

    /* Cas 1 : liste vide, ou bloc situé avant le premier bloc libre
       -> insertion en tête de liste */
    if (free_list == NULL || header < free_list) {
        header->ptr_next = free_list; 
        free_list = header;           
        return;
    }

    /* Cas 2 : on parcourt la liste pour trouver le dernier bloc libre
       dont l'adresse est inférieure à celle du bloc à insérer */
    HEADER *current = free_list;
    while (current->ptr_next != NULL && current->ptr_next < header)
        current = current->ptr_next;

    /* Insertion entre current et current->ptr_next. */
    header->ptr_next = current->ptr_next;
    current->ptr_next = header;
}
/* affichage de  la liste des blocs libres */
void print_free_list(void)
{
    printf("Free list:");
    for (HEADER *current = free_list; current != NULL; current = current->ptr_next)
        printf(" [%p, %zu bytes]", (void *)(current + 1), current->bloc_size);
    printf("\n");
}
