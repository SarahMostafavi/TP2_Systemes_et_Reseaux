#include "malloc_3is.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    return EXIT_SUCCESS;
}
