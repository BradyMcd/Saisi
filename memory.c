
#include <malloc.h>

#include "memory.h"

/**
 * Variable width, resizable allocations
 */

void *rw_malloc( size_t size ){
  return malloc( size );
}

void *rw_realloc( void* ptr, size_t size ){
  return realloc( ptr, size );
}

void rw_free( void *ptr ){
  free( ptr );
}

/**
 * Fixed width, recyclable allocations
 */

void *fw_malloc( size_t size ){
  return malloc( size );
}

void fw_free( void *ptr ){
  free( ptr );
}

/**
 * Variable width, write once allocations
 */

void *wo_malloc( size_t size ){
  return malloc( size );
}

void wo_free( void *ptr ){
  free( ptr );
}
