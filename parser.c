
#include <stdio.h>

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <malloc.h>

#include "tylist.h"

/* TODO: Convert to using the tylist interface */

#define TRY_ALLOC( RV, SIZE )                   \
  RV = malloc( SIZE );                          \
  if( RV == NULL ){ goto _CATCH;/* ENOMEM */ }

#define TRY_ALLOC_FN( RV, FN )                  \
  RV = FN();                                    \
  if( RV == NULL ){ goto _CATCH;/* ENOMEM */ }


#define TRY_REALLOC( CURR, SIZE )                 \
  void *_TEMP = realloc( CURR, SIZE );            \
  if( _TEMP == NULL ){ goto _CATCH;/* ENOMEM */ } \
  else{ CURR = _TEMP; }

#define CATCH _CATCH:

enum cClassTag{
  CC_EPS,
  CC_SINGLE,
  CC_RANGE,
  CC_SET,
};

struct charClass{
  enum cClassTag tag;
  union{
    char c;
    char r[2];
    char *buff;
  } data;
};

bool cc_match( struct charClass *class, char c ){

  if( class->tag == CC_EPS ){ return true; }
  if( c == '\0' ){ return false; } /* It makes most sense to make struct charClass be
                                      the thing which understands what a C string is*/
  int a = 0, b = 0;
  bool ret = 0;

  switch( class->tag ){
  case CC_SINGLE:
    return class->data.c == c;
    break;
  case CC_RANGE:
    // NOTE: Misordered ranges don't pass, police that elsewhere
    a = class->data.r[0] - c;
    b = class->data.r[1] - c;
    return ( a <= 0 ) && ( 0 <= b );
    break;
  case CC_SET:
    while( ret == false && class->data.buff[a] != '\0' ){
      ret = class->data.buff[a] == c;
      ++a;
    }
    return ret;
    break;
  default:
    return false;
  }
}

struct charClass *cc_range( char a, char b ){

  assert( a <= b );

  struct charClass *class;
  TRY_ALLOC( class, sizeof( struct charClass ) );

  class->tag = CC_RANGE;
  class->data.r[0] = ( a < b ) ? a : b;
  class->data.r[1] = ( a < b ) ? b : a;

  return class;

  CATCH
    return NULL;
}

struct charClass *cc_set( char *c ){

  struct charClass *class;
  TRY_ALLOC( class, sizeof( struct charClass ) );

  class->tag = CC_SET;
  TRY_ALLOC( class->data.buff, strlen( c ) + 1 );
  strncpy( class->data.buff, c, strlen( c ) + 1 );

  return class;

  CATCH{
    if( class != NULL ){ free( class ); }
    return NULL;
  }
}

struct charClass *cc_single( char c ){

  struct charClass *class;
  TRY_ALLOC( class, sizeof( struct charClass ) );

  class->tag = CC_SINGLE;
  class->data.c = c;

  return class;

  CATCH
    return NULL;
}

bool cc_iseps( struct charClass *class ){
  return class->tag == CC_EPS;
}

void cc_cleanup( struct charClass *class ){

  switch( class->tag ){
  case CC_SET:
    free( class->data.buff );
  case CC_SINGLE:
  case CC_RANGE:
    free( class );
  }
}

typedef size_t state_n;

typedef struct matcher matcher;
typedef struct relation{
  struct charClass *class;
  union{
    matcher *to;
    state_n n;
  } state;
  bool opt;
} relation;

void relation_cleanup( relation *ptr ){
  /* TODO: IMPLEMENT */
  /* NOTE: I don't think there is a halt and catch fire state which needs concordant free
   * I should be safe enough to just assume the state union is in state_n mode but I'm not sure */
}

TY_LIST_IFACE( relation, &relation_cleanup )
ITER_IFACE( relation );

typedef struct matcher{

  relation_list *arc;

  bool accept;
} matcher;

matcher *newState( ){
  matcher *ret;
  TRY_ALLOC( ret, sizeof( matcher ) );

  ret->arc = new_relation_list_sized( 4 );
  ret->accept = false;

  return ret;

  CATCH{
    if( ret != NULL ){ free(ret); }
    return NULL;
  }
}

bool relationAdd( matcher *s, struct charClass *c, bool opt, state_n to ){

  assert( s != NULL );
  assert( c != NULL );

  relation new_arc;

  TRY_ALLOC( new_arc, sizeof( relation ) );

  new_arc->class = c;
  new_arc->state.n = to;

  new_arc->opt = opt;

  return relation_add( s->arc, new_arc ); //Either returns TY_ERR_ENOMEM or TY_ERR_NONE
  /* Either returns TY_ERR_ENOMEM ( 0, false ) or TY_ERR_NONE ( 1, true ) */
  CATCH{
    return false;
  }
}

int relationFollow( relation *arc, char *rh ){

  bool matched = cc_match( arc->class, rh[0] );
  bool advance = !cc_iseps( arc->class ) && matched;

  if( arc->opt || matched ){
    return advance + sm_match( arc->state.to, &rh[advance] );
  }else{
    return -1;
  }
}

int sm_match( matcher *sm, char *rh ){

  int max = -1;
  int temp;
  int i;

  struct list_iter *iter = start_iter( sm->arc );
  relation *curr = iter_peek_relation( iter );
  while( curr != NULL ){
    temp = relationFollow( curr );
    max = ( temp>max )? temp: max;
    curr = iter_next_relation( iter );
  }
  end_iter( iter );

  if( max == -1 ){
    return max + sm->accept;
  }else{
    return max;
  }
}

//Returns the first match found
char *parseString( matcher *sm, char *string ){

  assert( string );
  assert( sm );

  int i = 0;
  int pos;
  char *ret = NULL;
  bool found = false;

  while( !found ){

    pos = sm_match( sm, &string[i]);
    found = pos > -1;
    i += !found;
  }

  if( found ){

    TRY_ALLOC( ret, pos + 1 );
    strncpy( ret, &string[i], pos );
    ret[pos] = '\0';
  }
  return ret;
  CATCH{
    return NULL; //? What else can I do here?
  }
}

/* Major disadvantages are coming to light here. NEED typedef'd types for the list interface
 * clutters the namespace far further per interface generated */

TY_LIST_IFACE( matcher, &dropnothing )
ITER_IFACE( group )

typedef struct group group;

TY_LIST_IFACE( group, &dropnothing )
ITER_IFACE( group )

typedef size_t state_n;

typedef struct group{

  matcher_list *states;
  group_list *sub;

  size_t s; /* First location in regex string */
  size_t e; /* Last  location in regex string */

  state_n considered;
  state_n next;
} group;

struct group *newGroup( ){

  struct group *ret;
  TRY_ALLOC( ret, sizeof( struct group ) );

  ret->states = new_matcher_list_sized( 4 );
  ret->subs = NULL;

  matcher_list_add( ret->states, newState( ) );
  matcher_list_add( ret->states, newState( ) );

  ret->considered = 0;
  ret->next = 2;

  return ret;

  CATCH{
    return NULL;
  }
}

/* Halt and Catch Fire */
void hcfGroup( group *grp ){

  if( grp == NULL ){
    return;
  }
  tylist_cleanup( grp->states );
  if( grp->subs != NULL ){
    tylist_cleanup( grp->subs );
  }
  free( grp );
}

struct group *outerGroup( struct group *g ){
  assert( g != NULL );
  matcher_list_get( g->states, 1 )->accept = true;
  return g;
}

group *addSub( group *p ){

  group *ret;

  if( p->subs == NULL ){
    TRY_ALLOC_FN( p->subs, new_group_list_sized( 2 ) );
  }

  TRY_ALLOC_FN( ret, newGroup( ) );

  if( group_add( p->subs, ret ) != TY_ERR_NONE ){
    hcfGroup( ret );
    return NULL;
  }

  return ret;

  CATCH{
    return NULL;
  }
}

struct matcher *g_currState( struct group *g ){
  assert( g != NULL );

  return matcher_get( g->states, considered );
}

state_n g_nextState( struct group *g ){
  assert( g != NULL );

  return g->next;
}

struct matcher *buildMatcher( char *regex ){

  // Recursive?
  struct group g = newGroup( );
  size_t idx = 0;

  /* I need a char stack? */
  char slot = 0;

  switch( regex[idx] ){
  case: '|': /* Pop slot and return to start of group */
    break;
  case '+': /* Pop slot and add an EPS transition from self to self */
    break;
  case '?': /* Pop slot and add the optional flag */
    break;
  case '\\': /* Escape character, same as default but +1 index */
    ++ idx;
  default: /* Stick whatever is here onto a slot, slot content pops to a charclass */
    if( slot == 0 ){
      slot = regex[idx];
    }else{
      relationAdd( g_currState( g ), cc_single( slot ), false, g_nextState( g ) );
      slot = regex[idx];
    }
    break;
  }
}

int main(){

  /* Matching parser for the regex string: /-?([0-9]+)|(ing|ed|es)/ */
  return 0;
}
