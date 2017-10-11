
#ifndef TYLIST_H
#define TYLIST_H

#include <stddef.h> /* size_t */
#include <stdbool.h>

#include <assert.h>

/* TODO: Attatch a foreach interface to the external iterator */
/* TODO: Ongoing testing */

typedef void (*dropfn)(void *drop);

enum tylist_err{
  TY_ERR_ENOMEM = 0,
  TY_ERR_NONE = 1,
  TY_ERR_BOUNDS,
};

bool ty_err_print( enum tylist_err );

#define INIT_CAP 10

void dropnothing( void *bot ){
  /* DONOTHING */
  /* NOTE: this is only useful if you have the objects you're storing tracked elsewhere as well
   * You should usually use &free as the drop function, since it's only used for cleanup and then only
   * in the case that the list dies before everything has been taken out which is probably an error
   * state.
   */
}

/* NOTE: The following interfaces are not recommended for external use */

struct generic_list{

  dropfn drop;

  size_t len;
  size_t cap;
  size_t width;
  void **data;
};

struct generic_list *_new_list( size_t, dropfn );
enum tylist_err _ty_set( struct generic_list*, void*, size_t );
enum tylist_err _ty_insert( struct generic_list*, void*, size_t );
enum tylist_err _ty_add( struct generic_list*, void* );
void *_ty_get( struct generic_list*, size_t );
void *_ty_remove( struct generic_list*, size_t );
void *_ty_pop( struct generic_list* );

struct list_iter{
  struct generic_list *lst;
  size_t i;
};

struct list_iter *start_iter( struct generic_list* );
void *_iter_peek( struct list_iter* );
void _iter_next( struct list_iter* );
void *_iter_get_next( struct list_iter* );
enum tylist_err _iter_insertion( struct list_iter*, void* );
void end_iter( struct list_iter* )

/* NOTE: Use the following interface externally */

#define TY_LIST_IFACE( TY, DROP )                                       \
  typedef struct generic_list* TY ## _list;                             \
  TY ## _list new_ ## TY ## _list( ){                                   \
    return (TY ## _list)_new_list( INIT_CAP, DROP );                    \
  }                                                                     \
  TY ## _list new_ ## TY ## _list_sized( size_t cap ){                  \
    return (TY ## _list)_new_list( cap, DROP );                         \
  }                                                                     \
  enum tylist_err TY ## _set( TY ## _list list, TY *pl, size_t idx ){   \
    return _ty_set( list, (void*)pl, idx );                             \
  }                                                                     \
  enum tylist_err TY ## _insert( TY ## _list list, TY *pl, size_t idx ){ \
    return _ty_insert(list, (void*)pl, idx );                           \
  }                                                                     \
  enum tylist_err TY ## _add( TY ## _list list, TY *pl ){               \
    return _ty_add( list, (void*)pl );                                  \
  }                                                                     \
  TY * TY ## _get( TY ## _list list, size_t idx ){                      \
    return (TY *)_ty_get( list, idx );                                  \
  }                                                                     \
  TY * TY ## _remove( TY ## _list list, size_t idx ){                   \
    return (TY *)_ty_remove( list, idx );                               \
  }                                                                     \
  TY * TY ## _pop( TY ## _list list ){                                  \
    return (TY *)_ty_pop( list );                                       \
  }

/* NOTE: foreach will all go here */
#define ITER_IFACE( TY )                                                \
  TY *iter_next_ ## TY ( struct list_iter *lst ){                       \
    return (TY *)_iter_get_next( lst );                                 \
  }                                                                     \
  TY *iter_peek_ ## TY (struct list_iter *lst){                         \
    return (TY *)_iter_peek( lst );                                     \
  }                                                                     \
  enum tylist_err iter_insert_ ## TY( struct list_iter *lst, TY *pl ){  \
    return _iter_insertion( lst, pl );                                  \
  }

void ty_err_print( enum ty_err );

size_t tylist_len( struct generic_list* );
enum tylist_err tylist_shrink( struct generic_list* );
void tylist_cleanup( struct generic_list* );

/* Concordant behaviours */
#ifndef NDEBUG
#define INVAR_CONCORD( TY, CMP )                        \
  void invar_concord_list( struct generic_list *list ){ \
    assert( list != NULL );                             \
    struct list_iter *iter = start_iter( list );        \
    TY *prev = NULL;                                    \
    TY *curr = iter_peek_ ## TY( iter );                \
    while( curr != NULL ){                              \
      assert( CMP( prev, curr ) < 0 );                  \
      prev = curr;                                      \
      curr = iter_next_ ## TY( iter );                  \
    }                                                   \
    end_iter( iter );                                   \
  }
#else
#define INVAR_CONCORD( TY, CMP )                        \
  void invar_concord_list( struct generic_list*list ){  \
    /* Do Nothing, NDEBUG set */                        \
  }
#endif

#define TY_LIST_CONCORD( TY, CMP )                                      \
  INVAR_CONCORD( TY, CMP )                                              \
  size_t TY ## _find( TY ## _list list, TY *pl ){                       \
    invar_concord_list( list );                                         \
    struct list_iter *iter = start_iter( list );                        \
    TY *curr = iter_peek_ ## TY( iter );                                \
    size_t ret;                                                         \
                                                                        \
    while( curr != NULL ){                                              \
      if( CMP( curr, pl ) == 0 ){                                       \
        ret = iter_index( iter );                                       \
        end_iter( iter );                                               \
        return ret;                                                     \
      }                                                                 \
      curr = iter_get_ ## TY( iter );                                   \
    }                                                                   \
                                                                        \
    return -1;                                                          \
  }                                                                     \
  enum tylist_err TY ## _concord_add( TY ## _list list, TY *pl ){       \
    invar_concord_list( list );                                         \
    struct list_iter *iter = start_iter( list );                        \
    TY *curr = iter_peek_ ## TY( iter );                                \
    enum tylist_err ret;                                                \
                                                                        \
    while( curr != NULL && CMP( curr, pl ) < 0 ){                       \
      curr = iter_get_ ## TY( iter );                                   \
    }                                                                   \
    ret = _iter_insertion( iter, pl );                                  \
    end_iter( iter );                                                   \
    return ret;                                                         \
  }                                                                     \

#endif//TYLIST_H
