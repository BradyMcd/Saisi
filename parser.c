
#include <stdio.h>

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "memory.h"

enum cClassTag{
  CC_SINGLE,
  CC_RANGE,
  CC_SET,
};

typedef struct charClass{
  enum cClassTag tag;
  union{
    char c;
    char r[2];
    char *buff;
  } data;
}charClass;


bool cc_match( charClass *class, char c ){

  if( class == NULL ){ return true; } /* Epsilon transition */
  if( c == '\0' ){ return false; } /* It makes most sense to make charClass be the thing
                                      which understands what a C string is*/
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

charClass *cc_range( char a, char b ){

  assert( a <= b );

  charClass *class = fw_malloc( sizeof( charClass ) );
  if( class == NULL ){ return NULL; }

  class->tag = CC_RANGE;
  class->data.r[0] = a;
  class->data.r[1] = b;

  return class;
}

charClass *cc_set( char *c ){

  charClass *class = fw_malloc( sizeof( charClass ) );
  if( class == NULL ){ return NULL; }

  class->tag = CC_SET;
  class->data.buff = wo_malloc( strlen( c ) + 1 );
  if( class->data.buff == NULL ){ fw_free( class ); return NULL; }
  strncpy( class->data.buff, c, strlen( c ) + 1 );

  return class;
}

charClass *cc_single( char c ){

  charClass *class = fw_malloc( sizeof( charClass ) );
  if( class == NULL ){ return NULL; }

  class->tag = CC_SINGLE;
  class->data.c = c;

  return class;
}

typedef struct matcher matcher;
typedef struct relation{
  charClass class;
  matcher *to;
  bool opt;
}

typedef struct matcher{
  int relations;
  relation **arc;

  bool accept;
}matcher;

int relationFollow( relation *arc, char *rh ){

  bool matched = cc_match( arc->class, rh[0] );
  bool advance = arc->class && matched;

  if( arc->opt || matched ){
    return advance + sm_match( arc->to, &rh[advance] );
  }else{
    return -1;
  }
}

int sm_match( matcher *sm, char *rh ){

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

    ret = wo_malloc( pos + 1 );
    strncpy( ret, &string[i], pos );
    ret[pos] = '\0';
  }
  return ret;
}

int main(){

  return 0;
}
