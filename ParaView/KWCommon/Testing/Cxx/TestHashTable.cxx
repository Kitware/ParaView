#include "vtkArrayMap.txx"
#include "vtkKWHashTable.h"
#include "vtkKWHashTableIterator.h"
#include "vtkStringArrayMap.txx"
#include "vtkString.h"
#include "vtkActor.h"

int main()
{
  int error = 0;
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

  /*
    vtkArrayMap<int,char*> *am 
    = vtkArrayMap<int,char*>::New();
    for ( cc = 9; cc >= 0; cc -- )
    {
    if ( am->SetItem(cc, names[cc]) != VTK_OK )
    {
    cout << "Problem adding item to the array map" << endl;
    error = 1;
    }
    }
    for ( cc = 0; cc < 10; cc ++ )
    {
    char *buffer = 0;
    if ( am->GetItem(cc, buffer) != VTK_OK)
    {
    cout << "Problem retrieving item from the array map" << endl;
    error = 1;
    }
    if ( !buffer || strcmp(buffer, names[cc]) )
    {
    cout << "Retrieved string: " << (buffer?buffer:"") 
    << " is not the same as the"
    " one inserted" << endl;
    }
    }
 
    am->Delete();
  */  
  vtkArrayMap<const char*,const char*> *sam 
    = vtkArrayMap<const char*,const char*>::New();
  //vtkStringArrayMap<const char*> *sam = vtkStringArrayMap<const char*>::New();
  vtkAbstractMapKeyIsString(sam);
  for ( cc = 9; cc >= 0; cc -- )
    {
    char *nkey = vtkString::Duplicate(names[cc]);
    if ( sam->SetItem(nkey, names[cc]) != VTK_OK )
      {
      cout << "Problem adding item to the array map" << endl;
      error = 1;
      }
    delete [] nkey;
    }
  sam->DebugList();
  for ( cc = 0; cc < 10; cc ++ )
    {
    char *buffer = 0;
    if ( sam->GetItem(names[cc], buffer) != VTK_OK)
      {
      cout << "Problem retrieving item from the array map" << endl;
      error = 1;
      }
    if ( !buffer )
      {
      cout << "Cannot access key: " << names[cc] << endl;
      }      
    else
      {
      if ( strcmp(buffer, names[cc]) )
	{
	cout << "Retrieved string: " << (buffer?buffer:"") 
	     << " is not the same as the"
	  " one inserted" << endl;
	error  = 1;
	}
      }
    }
  char *buffer = 0;
  if ( sam->GetItem("Brad", buffer) != VTK_OK)
    {
    cout << "Problem retrieving item from the array map" << endl;
    error = 1;
    }
  if ( !buffer )
    {
    cout << "Cannot access key: " << names[cc] << endl;
    error  = 1;
    }      
  else
    {
    if ( strcmp(buffer, "Brad") )
      {
      cout << "Retrieved string: " << (buffer?buffer:"") 
	   << " is not the same as the"
	" one inserted" << endl;
      error  = 1;
      }
    }
 
  sam->Delete();

  vtkArrayMap<const char*, vtkActor*> *soam 
    = vtkArrayMap<const char*, vtkActor*>::New();
  vtkAbstractMapKeyIsString(soam);
  vtkAbstractMapDataIsReferenceCounted(soam);
  char name[20];
  for( cc = 0; cc < 10; cc ++ )
    {
    sprintf(name, "actor%02d", cc);
    vtkActor* actor = vtkActor::New();
    if ( soam->SetItem(name, actor) != VTK_OK )
      {
      cout << "Problem inserting item in the map, key: " 
	   << name << " data: " << actor << endl;
      error  = 1;
      }

    vtkActor* nactor = 0;
    if ( soam->GetItem(name, nactor) != VTK_OK )
      {
      cout << "Problem accessing item in the map: " << name << endl;
      error  = 1;
      }
    if ( !nactor )
      {
      cout << "Item: " << name << " should not be null" << endl;
      error  = 1;
      }
    if ( nactor != actor )
      {
      cout << "Item: " << nactor << " at key: " << name 
	   << " is not the same as: " << actor << endl;
      error  = 1;
      }
    
    actor->Delete();
    }

  soam->Delete();

  vtkKWHashTable *ht = vtkKWHashTable::New();

  for ( cc =0 ; cc< 10; cc++ )
    {
    unsigned long key = vtkKWHashTable::HashString( names[cc] );
    int res = ht->Insert( names[cc], (void *)names[cc] );
    if ( !res )
      {
      cout << "Insert: " << names[cc] << " at key: " << key 
	   << (res ? " ok" : " problem") << endl;
      error = 1;
      }
    }

  for ( cc =0 ; cc< 10; cc++ )
    {
    unsigned long key = vtkKWHashTable::HashString( names[cc] );
    char *name = ( char * )ht->Lookup( names[cc] );
    if ( name ) 
      {
      }
    else
      {      
      cout << key << ": does not exist" << endl;
      error = 1;
      }
    }
  
  for ( cc=0; cc<10; cc++ )
    {
    ht->Remove( names[cc] );
    }

  for ( cc=0; cc<10; cc++) 
    {
    unsigned long key = vtkKWHashTable::HashString( names[cc] );
    int res = ht->Insert( names[cc], (void *)names[cc] );
    if ( !res )
      {
      cout << "Insert: " << names[cc] << " at key: " << key 
	   << (res ? " ok" : " problem") << endl;
      error = 1;
      }
    }

  vtkKWHashTableIterator *it = ht->Iterator();
  if ( !it )
    {
    cout << "Cannot get the pointer. This is strange, since I just"
      "inserted something..." << endl;
    error = 1;
    }
  while ( it )
    {
    if ( it->Valid() )
      {      
      unsigned long key = it->GetKey();
      void *value = it->GetData();
      if ( !value )
	{
	cout << "Key: " << key << " store element: " << value 
	     << " (" << static_cast<char *>(value) << ")" << endl;
	error = 1;
	}
      }
    if ( !it->Next() )
      {
      break;
      }
    }
  if ( it )
    {
    it->Delete();
    }

  ht->Delete();

  return error;
}

