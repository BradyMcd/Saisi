
#include <stdio.h>

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <malloc.h>

#define TRY_ALLOC( RV, SIZE ) \
  RV = malloc( SIZE );        \
  if( RV == NULL ){ /* ENOMEM */ }

#define TRY_REALLOC( CURR, SIZE )               \
  void *_TEMP = realloc( CURR, SIZE );          \
  if( _TEMP == NULL ){ /* ENOMEM */ }           \
  else{ CURR = _TEMP; }

#define CATCH _CATCH:

enum cClassTag{
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

  if( class == NULL ){ return true; } /* Epsilon transition */
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
}

struct charClass *cc_set( char *c ){

  struct charClass *class;
  TRY_ALLOC( class, sizeof( struct charClass ) );

  class->tag = CC_SET;
  class->data.buff = malloc( strlen( c ) + 1 );
  if( class->data.buff == NULL ){ free( class ); return NULL; }
  strncpy( class->data.buff, c, strlen( c ) + 1 );

  return class;
}

struct charClass *cc_single( char c ){

  struct charClass *class;
  TRY_ALLOC( class, sizeof( struct charClass ) );

  class->tag = CC_SINGLE;
  class->data.c = c;

  return class;
}

typedef size_t state_n;

struct matcher;
struct relation{
  struct charClass *class;
  union{
    struct matcher *to;
    state_n n;
  } state;
  bool opt;
};

struct matcher{
  int relations;
  struct relation **arc;

  bool accept;
};

struct matcher *newState( ){
  struct matcher *ret;
  TRY_ALLOC( ret, sizeof( struct matcher ) );

  ret->relations = 0;
  ret->arc = NULL;
  ret->accept = false;

  return ret;
}

void relationAdd( struct matcher *s, struct charClass *c, bool opt, state_n to ){

  assert( s != NULL );
  assert( c != NULL );

  TRY_REALLOC( s->arc, sizeof( struct relation* ) * ( s->relations + 1 ) );

  struct relation *new_arc;
  TRY_ALLOC( new_arc, sizeof( struct relation ) );

  new_arc->class = c;
  new_arc->state.n = to;

  new_arc->opt = opt;

  s->relation[s->relations] = new_arc;
  s->relations += 1
}

int relationFollow( struct relation *arc, char *rh ){

  bool matched = cc_match( arc->class, rh[0] );
  bool advance = arc->class && matched;

  if( arc->opt || matched ){
    return advance + sm_match( arc->state.to, &rh[advance] );
  }else{
    return -1;
  }
}

int sm_match( struct matcher *sm, char *rh ){

  int max = -1;
  int temp;
  int i;

  for( i = 0; i < sm->relations; ++i ){
    temp = relationFollow( sm->arc[i], rh );

    max = ( temp > max )? temp: max;
  }

  if( max == -1 ){
    return max + sm->accept;
  }else{
    return max;
  }
}

//Returns the first match found
char *parseString( struct matcher *sm, char *string ){

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

    ret = malloc( pos + 1 );
    strncpy( ret, &string[i], pos );
    ret[pos] = '\0';
  }
  return ret;
}

typedef size_t state_n;

struct group{

  size_t states;
  struct matcher **state;

  size_t subs;
  struct group **sub;

  state_n considered;
  state_n next;
};

struct group *newGroup( ){

  struct group *ret;
  TRY_ALLOC( ret, sizeof( struct group ) );

  TRY_ALLOC( ret->state, sizeof( struct matcher* ) * 2 );

  ret->state[0] = newState( );
  ret->state[1] = newState( );
  ret->states = 2;

  ret->subs = 0;
  ret->sub = NULL;

  ret->considered = 0;
  ret->next = 2;

  return ret;
}

struct group *outerGroup( struct group *g ){
  assert( g != NULL );
  g->state[1]->accept = true;
  return g;
}

struct group *addSub( struct group *p ){

  struct group** ret = realloc( p->sub, ( p->subs + 1 ) * sizeof( struct group* ) );
  if( ret == NULL ){ /* ENOMEM */ }
  ret[p->subs] = newGroup();

  p->subs += 1;

  return ret->[p->subs - 1];
}

struct group *intoGroup( char *curr ){

  struct group

}

struct matcher *buildMatcher( char *regex ){

  // Recursive?
  struct group g = newGroup( );

  switch( regex[0] ){
  case '\\': /* Escape character, same as default but +1 index */
    break;
  case: '|': /* Pop slot and return to start of group */
    break;
  case '+': /* Pop slot and add an EPS transition from self to self */
    break;
  case '?': /* Pop slot and add the optional flag */
    break;
  default: /* Stick whatever is here onto a slot, slot content pops to a charclass */
    break;
  }
}



int main(){

  /* Matching parser for the regex string: /-?([0-9]+)|(ing|ed|es)/ */
  return 0;
}
