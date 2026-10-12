#include <stddef.h>

typedef struct HEADER_TAG {
    struct HEADER_TAG *ptr_next; /* pointe sur le prochain bloc libre */
    size_t bloc_size;             /* taille du memory bloc en octets */
    long magic_number;            /* nombre magique */
} HEADER;

void *malloc_3is(size_t size);

#define MAGIC_NUMBER 0xBADA110CBADA110CL
