
#include <malloc.h>
#include <stdbool.h>
#include <string.h>

#include <stdio.h>

#include <assert.h>
#include <errno.h>

typedef void (*dropfn)(void *drop);

enum tylist_err{
  TY_ERR_ENOMEM = 0,
  TY_ERR_NONE = 1,
  TY_ERR_BOUNDS,
};

#define TRY_ALLOC( RV, SIZE )                   \
  errno = 0;                                    \
  RV = malloc( SIZE );                          \
  if( RV == NULL ){ return NULL; }

#define TRY_REALLOC( CURR, SIZE ){               \
    errno = 0;                                   \
    void *_TEMP = realloc( CURR, SIZE );         \
    if( _TEMP == NULL ){ return TY_ERR_ENOMEM; } \
    else{ CURR = _TEMP; } }

struct generic_list{

  dropfn drop;

  size_t len;
  size_t cap;
  size_t width;
  void **data;
};

bool ty_err_print( enum tylist_err err ){

  switch( err ){
  case TY_ERR_ENOMEM:
    fprintf( stderr, "List allocation or reallocation failed\n" );
    return true;
    break;
  case TY_ERR_BOUNDS:
    fprintf( stderr, "Index out of bounds error\n" );
    return true;
    break;
  case TY_ERR_NONE:
    fprintf( stderr, "No error reported\n" );
    break;
  default:
    fprintf( stderr, "Unknown error number %u\n", err );
    break;
  }
  return false;
}

struct generic_list *_new_list( size_t cap, dropfn drop ){
  assert( cap > 0 );
  assert( drop != NULL );

  struct generic_list *ret;

  TRY_ALLOC( ret, sizeof( struct generic_list ) );
  TRY_ALLOC( ret->data, ( cap * sizeof( void* ) ) );

  ret->drop = drop;

  ret->len = 0;
  ret->cap = cap;

  return ret;
}

enum tylist_err grow( struct generic_list *list ){
  assert( list != NULL );

  size_t newcap = list->cap * 2;
  TRY_REALLOC( list->data, ( newcap * sizeof( void* ) ) );
  list->cap = newcap;
  return TY_ERR_NONE;
}

enum tylist_err _ty_set( struct generic_list *list, void *pl, size_t idx ){
  assert( list != NULL );
  assert( pl != NULL );
  assert( idx >= 0 );

  if( idx > list->len ){ return TY_ERR_BOUNDS; }
  if( idx == list->len && list->cap == list->len ){
    if( grow( list ) != TY_ERR_NONE ){
      return TY_ERR_ENOMEM;
    }
  }

  list->data[idx] = pl;

  if( idx == list->len ){
    list->len += 1;
  }

  return TY_ERR_NONE;
}

enum tylist_err _ty_insert( struct generic_list *list, void *pl, size_t idx ){
  assert( list != NULL );
  assert( pl != NULL );
  assert( idx >= 0 );

  if( idx > list->len ){ return TY_ERR_BOUNDS; }
  if( list->len == list->cap ){
    if( grow( list ) != TY_ERR_NONE ){
      return TY_ERR_ENOMEM;
    }
  }

  memmove( list->data[idx + 1], list->data[idx], list->len - idx );
  list->data[idx] = pl;

  list->len += 1;

  return TY_ERR_NONE;
}

enum tylist_err _ty_add( struct generic_list *list, void *pl ){
  assert( list != NULL );
  assert( pl != NULL );

  return _ty_set( list, pl, list->len );
}

void *_ty_get( struct generic_list *list, size_t idx ){

  if( idx > list->len ){ return NULL; }

  return list->data[idx];
}

void *_ty_remove( struct generic_list *list, size_t idx ){
  void *ret;

  ret = _ty_get( list, idx );
  if( ret == NULL ){ return NULL; }
  if( idx < ( list->len - 1 ) ){
    memmove( &list->data[idx], &list->data[idx + 1], sizeof( void* ) * ( list->len - ( idx + 1 ) ) );
  }
  list->len = list->len - 1;
  return ret;
}

void *_ty_pop( struct generic_list *list ){

  if( list->len == 0 ){
    return NULL;
  }else{
    return _ty_remove( list, list->len - 1 );
  }
}

/* Iterator */

struct list_iter{
  struct generic_list *lst;
  size_t i;
};

struct list_iter *start_iter( struct generic_list *lst ){

  struct list_iter *ret;

  TRY_ALLOC( ret, sizeof( struct list_iter ) );

  ret->lst = lst;
  ret->i = 0;

  return ret;
}

size_t tylist_len( struct generic_list* );

void *_iter_peek( struct list_iter *iterator ){

  if( iterator->i == -1 ){
    return NULL;
  }
  return _ty_get( iterator->list, iterator->i );
}

bool iter_next( struct list_iter *iterator ){

  if( iterator->i == tylist_len( iterator->lst ) ){
    iterator->i = -1;
    return false;
  }
  ++ iterator->i;
  return true;
}

void *_iter_get_next( struct list_iter *iterator ){

  iter_next( iterator );
  return _iter_peek( iterator );
}

enum tylist_err _iter_insertion( struct list_iter *iterator, void *pl ){

  enum tylist_err ret;

  ret = _ty_insert( iterator->list, pl, iterator->i );

  if( ret == TY_ERR_NONE ){
    ++ iterator->i;
  }
  return ret;
}

void end_iter( struct list_iter *iterator ){
  free( iterator );
}

/* Interface */
size_t tylist_len( struct generic_list *list ){
  return list->len;
}

enum tylist_err tylist_shrink( struct generic_list *list ){
  if( list->len < list->cap ){
    TRY_REALLOC( list->data, list->len * sizeof( void* ) );
  }
  return TY_ERR_NONE;
}

void tylist_cleanup( struct generic_list *list ){
  int i;
  for( i = 0; i < list->len; ++i ){
    list->drop(list->data[i]);
  }
  free( list->data );
  free( list );
}
