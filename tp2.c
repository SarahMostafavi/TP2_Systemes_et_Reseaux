#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>

typedef struct HEADER_TAG {
    struct HEADER_TAG *ptr_next;  /* prochain bloc libre */
    size_t bloc_size;             /* taille du memory bloc en octets */
    long magic_number;
} HEADER;

#define MAGIC_NUMBER ((long)0xBADA110CBADA110CUL)

/* Liste des blocs libres, triée par adresse croissante */
static HEADER *free_list = NULL;

/* Renvoie 1 si les deux magic numbers du bloc sont intacts, 0 sinon */
static int check_magic(HEADER *header)
{
    char *memory_bloc = (char *)(header + 1);
    long *end_magic = (long *)(memory_bloc + header->bloc_size);

    if (header->magic_number != MAGIC_NUMBER) {
        fprintf(stderr, "Erreur : header corrompu (bloc %p)\n",
                (void *)memory_bloc);
        return 0;
    }
    if (*end_magic != MAGIC_NUMBER) {
        fprintf(stderr, "Erreur : debordement detecte (bloc %p)\n",
                (void *)memory_bloc);
        return 0;
    }
    return 1;
}

/* Retire de la liste un bloc libre de taille size et le renvoie,
   ou renvoie NULL s'il n'y en a pas */
static HEADER *take_free_bloc(size_t size)
{
    HEADER *previous = NULL;
    HEADER *current = free_list;

    while (current != NULL) {
        if (current->bloc_size == size) {
            if (previous == NULL)
                free_list = current->ptr_next;  /* c'était le premier */
            else
                previous->ptr_next = current->ptr_next;

            current->ptr_next = NULL;
            return current;
        }
        previous = current;
        current = current->ptr_next;
    }
    return NULL;
}

void *malloc_3is(size_t size)
{
    if (size == 0) {
        return NULL;
    }

    /* On réutilise un bloc libre de même taille s'il en existe un */
    HEADER *reused = take_free_bloc(size);
    if (reused != NULL && check_magic(reused)) {
        return reused + 1;
    }

    /* Sinon on agrandit le tas : header + memory bloc + magic de fin */
    HEADER *header = sbrk(sizeof(HEADER) + size + sizeof(MAGIC_NUMBER));
    if (header == (void *)-1) {
        perror("sbrk");
        return NULL;
    }

    header->ptr_next = NULL;
    header->bloc_size = size;
    header->magic_number = MAGIC_NUMBER;

    char *memory_bloc = (char *)(header + 1);
    long *end_magic = (long *)(memory_bloc + size);
    *end_magic = MAGIC_NUMBER;

    return memory_bloc;
}

void free_3is(void *ptr)
{
    if (ptr == NULL)
        return;

    /* Le header est juste avant le memory bloc */
    HEADER *header = (HEADER *)ptr - 1;

    /* Un bloc corrompu n'est pas remis dans la liste */
    if (!check_magic(header))
        return;

    /* Insertion en tête si la liste est vide ou si le bloc est le plus bas */
    if (free_list == NULL || header < free_list) {
        header->ptr_next = free_list;
        free_list = header;
        return;
    }

    /* Recherche du dernier bloc libre d'adresse inférieure */
    HEADER *current = free_list;
    while (current->ptr_next != NULL && current->ptr_next < header)
        current = current->ptr_next;

    /* On relie d'abord le bloc au suivant pour ne pas perdre son adresse */
    header->ptr_next = current->ptr_next;
    current->ptr_next = header;
}

void print_free_list(void)
{
    printf("Free list:");
    for (HEADER *current = free_list; current != NULL;
         current = current->ptr_next)
        printf(" [%p, %zu bytes]", (void *)(current + 1), current->bloc_size);
    printf("\n");
}

int main(void)
{
    size_t size_a = 10;
    size_t size_b = 20;
    size_t size_c = 30;

    /* Etape 1 : allocations multiples */
    char *a = malloc_3is(size_a);
    char *b = malloc_3is(size_b);
    char *c = malloc_3is(size_c);

    if (a == NULL || b == NULL || c == NULL) {
        printf("malloc_3is failed\n");
        return EXIT_FAILURE;
    }

    strcpy(a, "bonjour");
    strcpy(b, "hello");
    strcpy(c, "salut");

    printf("a = %p : %s 	|pour size = %zu\n", (void *)a, a, size_a);
    printf("b = %p : %s 	|pour size = %zu\n", (void *)b, b, size_b);
    printf("c = %p : %s 	|pour size = %zu\n", (void *)c, c, size_c);

    /* Etape 2 : libération dans le désordre, la liste doit rester triée */
    printf("\n--- Test de free_3is ---\n");
    print_free_list();

    free_3is(b);
    printf("Apres free(b) : ");
    print_free_list();

    free_3is(c);
    printf("Apres free(c) : ");
    print_free_list();

    free_3is(a);
    printf("Apres free(a) : ");
    print_free_list();

    /* Etape 3 : débordement volontaire
       (taille 40 absente de la liste, pour avoir un bloc neuf) */
    printf("\n--- Test de debordement ---\n");
    size_t size_d = 40;
    char *d = malloc_3is(size_d);
    if (d == NULL) {
        printf("malloc_3is failed\n");
        return EXIT_FAILURE;
    }

    memset(d, 'X', size_d + sizeof(long));  /* écrase le magic de fin */

    free_3is(d);
    printf("Apres free(d) : ");
    print_free_list();

    /* Etape 4 : réutilisation d'un bloc libéré */
    printf("\n--- Test de reutilisation ---\n");

    char *e = malloc_3is(size_b);  /* même taille que b */
    printf("e = %p (b = %p) -> %s\n", (void *)e, (void *)b,
           e == b ? "bloc de b reutilise" : "nouveau bloc");
    print_free_list();

    char *f = malloc_3is(15);  /* aucune taille 15 dans la liste */
    printf("f = %p -> nouveau bloc attendu\n", (void *)f);
    print_free_list();

    return EXIT_SUCCESS;
}
