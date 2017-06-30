
#include <stdio.h>

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "memory.h"

enum cClassTag{
  CC_EPS,
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

  bool opt;
}charClass;


bool cc_match( charClass *class, char c ){
  assert( class );

  int a = 0, b = 0;
  bool ret = 0;

  switch( class->tag ){
  case CC_EPS:
    assert( class->opt == true );
    return true;
    break;
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

typedef struct matcher{
  int relations;
  struct matcher **relation;
  charClass **class;

  bool accept;
}matcher;


// NOTE: This technically has an error, it returns a number off by 1
int match( matcher *sm, char *rh ){

  assert( sm );
  assert( rh );

  int i;
  int ret = sm->accept;
  int temp = 0;
  bool accepted = false;

  if( rh[0] == '\0' ){return sm->accept;}

  for( i = 0; i < sm->relations; ++i ){
    accepted = cc_match( sm->class[i], *rh );

    if( accepted || sm->class[i]->opt ){
      temp = match( sm->relation[i], &rh[accepted] );
      if( temp > 0 ){
        ret = ( ret > temp ) ? ret : temp + accepted;
      }
    }
  }

  return ret;
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

    pos = match( sm, &string[i]);
    found = pos > 0;
    i += !found;
  }

  if( found ){

    ret = wo_malloc( pos );
    strncpy( ret, &string[i], pos - 1 );
    ret[pos - 1] = '\0';
  }
  return ret;
}

matcher *buildNumberMatcher(){

  matcher *states[3];
  charClass *classes[2];

  states[0] = fw_malloc( sizeof( matcher ) );
  states[1] = fw_malloc( sizeof( matcher ) );
  states[2] = fw_malloc( sizeof( matcher ) );

  classes[0] = cc_single( '-' );
  classes[0]->opt = true;

  classes[1] = cc_range( '0', '9' );

  states[0]->relations = 1;
  states[0]->relation = wo_malloc( sizeof(matcher*) );
  states[0]->class = wo_malloc( sizeof(charClass*) );
  states[0]->class[0] = classes[0];
  states[0]->accept = false;

  states[1]->relations = 1;
  states[1]->relation = wo_malloc( sizeof(matcher*) );
  states[1]->class = wo_malloc( sizeof(charClass*) );
  states[1]->class[0] = classes[1];
  states[1]->accept = false;

  states[2]->relations = 1;
  states[2]->relation = wo_malloc( sizeof(matcher*) );
  states[2]->class = wo_malloc( sizeof(charClass*) );
  states[2]->class[0] = classes[1];
  states[2]->accept = true;

  states[0]->relation[0] = states[1];
  states[1]->relation[0] = states[2];
  states[2]->relation[0] = states[2];

  return states[0];
}


int main(){

  matcher *state_machine = buildNumberMatcher();
  char test[18] = "Hell1992o World!";

  printf( "%s contains the number %s\n", test, parseString( state_machine, test ) );

  return 0;
}
