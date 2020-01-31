#include <stdlib.h>
#include <stdio.h>

int main()
{

  printf("Running test 2 to exercise malloc and free\n");

  char * ptr = ( char * ) malloc ( 65535 );

  char * ptr_array[1024];

  int i;
  for ( i = 0; i < 2; i++ )
  {
    ptr_array[i] = ( char * ) malloc ( 1024 );

    ptr_array[i] = ptr_array[i];
  }

  free( ptr );

  for ( i = 0; i < 2; i++ )
  {
    if( i % 2 == 0 )
    {
      free( ptr_array[i] );
    }
  }

  for ( i = 0; i < 2; i++ )
  {
    if( 1 )
    {
      ptr_array[i] = ( char * ) realloc( ptr_array[i], 2048 );
    }
    else
    {
      ptr_array[i] = ( char * ) calloc( 5, 1024 );
    }
  }

  ptr = ( char * ) malloc ( 65535 );
  free( ptr );

  return 0;
}
