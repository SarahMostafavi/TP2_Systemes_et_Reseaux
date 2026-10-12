#include <stdio.h>
#include <unistd.h>
#include "malloc_3is.h"

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
