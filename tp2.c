#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>


// Header
typedef struct HEADER_TAG {
    struct HEADER_TAG *ptr_next; /* pointe sur le prochain bloc libre */
    size_t bloc_size;             /* taille du memory bloc en octets */
    long magic_number;            /* nombre magique */
} HEADER;


#define MAGIC_NUMBER ((long)0xBADA110CBADA110CUL)

/* Tête de la liste chaînée des blocs libres, triée par adresse croissante. */
static HEADER *free_list = NULL;


// Réecriture du  malloc
void *malloc_3is(size_t size){
        // malloc renvoie NULL si size est 0
        if (size == 0){
                return NULL;
        }

        // demande de plus d'espace de la taille du header avec le memory bloc
        // et le magic number
        HEADER *header = sbrk(sizeof(HEADER) + size + sizeof(MAGIC_NUMBER));

        if (header == (void *) -1){
                perror("sbrk");
                return NULL;
        }

        // Initialisation du header
        header->ptr_next = NULL;
        header->bloc_size = size;
        header->magic_number = MAGIC_NUMBER;

        // Definition du memory bloc
        char* memory_bloc = (char*)(header+1);

        // Initialisation du nombre magique
        long *magic_number = (long *) (memory_bloc + size);
        *magic_number = MAGIC_NUMBER;

        return memory_bloc;
}
/* Vérifie les deux magic numbers d'un bloc.
   Renvoie 1 si le bloc est intact, 0 s'il y a eu un débordement */
static int check_magic(HEADER *header)
{
    char *memory_bloc = (char *)(header + 1);                    /* début du memory bloc */
    long *end_magic = (long *)(memory_bloc + header->bloc_size); /* juste après le memory bloc */

    if (header->magic_number != MAGIC_NUMBER) {
        fprintf(stderr, "Erreur : header corrompu (bloc %p)\n", (void *)memory_bloc);
        return 0;
    }
    if (*end_magic != MAGIC_NUMBER) {
        fprintf(stderr, "Erreur : debordement detecte (bloc %p)\n", (void *)memory_bloc);
        return 0;
    }
    return 1;
}



//Reecriture du free
void free_3is(void *ptr)
{
    /* free(NULL) ne fait rien */
    if (ptr == NULL)
        return;

    /* ptr pointe sur le memory bloc : le header est juste avant */
    HEADER *header = (HEADER *)ptr - 1;


    if (!check_magic(header))
        return;


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




int main(void) {
    // Fait 3 allocations
    size_t size_a = 10;
    size_t size_b = 20;
    size_t size_c = 30;

    char *a = malloc_3is(size_a);
    char *b = malloc_3is(size_b);
    char *c = malloc_3is(size_c);

    if (a == NULL || b == NULL || c == NULL) {
        printf("malloc_3is failed\n");
        return EXIT_FAILURE;
    }

    // Ecriture dans les memory_bloc
    strcpy(a, "bonjour");
    strcpy(b, "hello");
    strcpy(c, "salut");

    // Verification de l'ecriture
    printf("a = %p : %s 	|pour size = %ld\n", (void *)a, a, size_a);
    printf("b = %p : %s 	|pour size = %ld\n", (void *)b, b, size_b);
    printf("c = %p : %s 	|pour size = %ld\n", (void *)c, c, size_c);

     // Liberation 
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
    // Detection des debordements
    printf("\n--- Test de debordement ---\n");
    size_t size_d = 20;
    char *d = malloc_3is(size_d);
    if (d == NULL) {
        printf("malloc_3is failed\n");
        return EXIT_FAILURE;
    }

    // On ecrit 8 octets de trop : le magic number de fin est ecrase
    memset(d, 'X', size_d + sizeof(long));

    free_3is(d);            
    printf("Apres free(d) : ");
    print_free_list();     

    return EXIT_SUCCESS;
}
