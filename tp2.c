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
#define BLOC_OVERHEAD (sizeof(HEADER) + sizeof(long))	/* Place prise par un header et un magic de fin*/

static HEADER *free_list = NULL; /* Liste des blocs libres, triée par adresse croissante */





/* Fonction de vérification des débordements de mémoire 
  Renvoie 1 si les deux magic numbers du bloc sont intacts, 0 sinon */
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


/* Fonction d'écriture du magic number juste après le memory bloc */
static void set_end_magic(HEADER *header)
{
    char *memory_bloc = (char *)(header + 1);
    long *end_magic = (long *)(memory_bloc + header->bloc_size);
    *end_magic = MAGIC_NUMBER;
}

/* Découpe un bloc libre : sa fin devient un bloc de taille size, renvoyé.
   Le début reste dans la liste avec une taille diminuée. */
static HEADER *split_bloc(HEADER *free_bloc, size_t size)
{
    free_bloc->bloc_size -= size + BLOC_OVERHEAD;
    set_end_magic(free_bloc);

    /* Le nouveau bloc commence juste après le magic de fin du bloc libre */
    HEADER *new_bloc = (HEADER *)((char *)(free_bloc + 1)
                                  + free_bloc->bloc_size + sizeof(long));
    new_bloc->ptr_next = NULL;
    new_bloc->bloc_size = size;
    new_bloc->magic_number = MAGIC_NUMBER;
    set_end_magic(new_bloc);

    return new_bloc;
}

/* Cherche un bloc libre utilisable pour size octets :
   - taille exacte : il est retiré de la liste et renvoyé
   - assez grand : il est découpé (voir split_bloc)
   Renvoie NULL si aucun bloc ne convient. */
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
        /* Assez grand pour qu'il reste au moins 1 octet libre après découpage */
        if (current->bloc_size >= size + BLOC_OVERHEAD + 1)
            return split_bloc(current, size);

        previous = current;
        current = current->ptr_next;
    }
    return NULL;
}


/* Fonction qui détermine si deux blocs sont adjacents à partir de leurs header 
   Renvoie si le bloc b commence juste après la fin du bloc a*/
static int are_blocs_adjacent(HEADER *a, HEADER *b){
    char *end_of_a = (char *)(a + 1) + a->bloc_size + sizeof(long);
    return end_of_a == (char *)b;
}

/* Fonction pour absorber le bloc suivant dans le bloc en entrée. 
   Le header et le magic de fin du bloc absorbé deviennent de la mémoire utilisable.
   Le magic de fin de next devient celui du bloc fusionné. */
static void merge_bloc_with_next(HEADER *bloc)
{
    HEADER *next = bloc->ptr_next;

    bloc->bloc_size += BLOC_OVERHEAD + next->bloc_size;
    bloc->ptr_next = next->ptr_next;
}

/* Réecriture de la fonction malloc*/
void *malloc_3is(size_t size)
{
    if (size == 0) {
        return NULL;
    }

    /* On réutilise ou on découpe un bloc libre s'il en existe un */
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
    set_end_magic(header);

    return header + 1;
}

/* Réecriture de la fonction free*/
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

    /* Fusion avec le suivant, puis avec le précédent */
    if (header->ptr_next != NULL && are_blocs_adjacent(header, header->ptr_next))
        merge_bloc_with_next(header);
    if (are_blocs_adjacent(current, header))
        merge_bloc_with_next(current);
}

/* Affiche la liste de blocs free */
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

    char *f = malloc_3is(15);  /* aucun bloc de 15, ni assez grand à découper */
    printf("f = %p -> nouveau bloc attendu\n", (void *)f);
    print_free_list();

    /* Etape 5 : découpage d'un gros bloc libre */
    printf("\n--- Test de decoupage ---\n");

    char *h = malloc_3is(100);
    free_3is(h);
    printf("Apres free(h) : ");
    print_free_list();

    /* Pas de bloc de 20, mais h (100) peut être découpé :
       h garde 100 - 20 - 32 = 48 octets, g est placé à h + 48 + 8 + 24 */
    char *g = malloc_3is(20);
    printf("g = %p (attendu : h + 80 = %p)\n", (void *)g, (void *)(h + 80));
    print_free_list();

    return EXIT_SUCCESS;
}
