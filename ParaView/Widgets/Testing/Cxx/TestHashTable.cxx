#include "vtkKWHashTable.h"
#include <iostream.h>

int main()
{
  int cc;
  char names[][10] = {
    "Andy",
    "Amy",
    "Berk",
    "Bill",
    "Brad",
    "Charles",
    "Dina",
    "Ken",
    "Lisa",
    "Sebastien",
    "Will"
  };

  cout << "Testing KW Hash Table" << endl;

  vtkKWHashTable *ht = vtkKWHashTable::New();

  for ( cc =0 ; cc< 10; cc++ )
    {
    unsigned long key = vtkKWHashTable::HashString( names[cc] );
    int res = ht->Insert( names[cc], (void *)names[cc] );
    cout << "Insert: " << names[cc] << " at key: " << key 
	 << (res ? " ok" : " problem") << endl;
    }

  for ( cc =0 ; cc< 10; cc++ )
    {
    unsigned long key = vtkKWHashTable::HashString( names[cc] );
    char *name = ( char * )ht->Lookup( names[cc] );
    if ( name ) 
      {
      cout << key << ": " << name << endl;
      }
    else
      {
      cout << key << ": does not exist" << endl;
      }
    }

  for ( cc=0; cc<10; cc++ )
    {
    unsigned long key = vtkKWHashTable::HashString( names[cc] );
    ht->Remove( names[cc] );
    }

  ht->Delete();

  return 0;
}

