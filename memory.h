
#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

// Varible width, resizable allocations
void *rw_malloc( size_t );
void *rw_realloc( void*, size_t );
void rw_free( void* );

// Fixed width, recycling allocations
void *fw_malloc( size_t );
void fw_free( void* );

// Variable width, write once allocations
void *wo_malloc( size_t );
void wo_free( void* );

#endif//MEMORY_H
