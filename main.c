#include "malloc_3is.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "free_3is.h"
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



    return EXIT_SUCCESS;
}
