#include "vtkKWPointerArray.h"

int main()
{
  unsigned int cc;
  int error = 0;
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

  char separate[] = "separate";


  vtkKWPointerArray *pa = vtkKWPointerArray::New();

  for ( cc =0 ; cc< 10; cc++ )
    {
    int res = pa->Append( (void *)names[cc] );
    if ( !res )
      {
      cout << "Insert: " << names[cc] << " at position: " << cc
	   << (res ? " ok" : " problem") << endl;
      error = 1;
      }
    }

  for ( cc=0; cc < 11; cc++ )
    {
    char *name = (char *)pa->Lookup(cc);
    if ( name )
      {
      }
    else
      {
      if ( cc < 10 )
	{
	cout << "At position: " << cc << " there is no name" 
	     << endl;
	error = 1;
	}
      }
    }

  for ( cc=1; cc<10; cc+= 2 )
    {
    pa->Remove(cc);
    }

  for ( cc=0; cc < 11; cc++ )
    {
    char *name = (char *)pa->Lookup(cc);
    if ( name )
      {
      }
    else
      {
      if ( cc < 7 )
	{
	cout << "At position: " << cc << " there is no name" 
	     << endl;
	error = 1;
	}
      }
    }
  
  if ( pa->GetSize() != 7 )
    {
    cout << "Number of elements left: " << pa->GetSize() << endl;
    error = 1;
    }
  for ( cc =0 ; cc< 100; cc++ )
    {
    int res = pa->Prepend( (void *)separate );
    if ( !res )
      {
      cout << "Prepend: " << separate << " at position: " << 0
	   << (res ? " ok" : " problem") << endl;
      error = 1;
      }
    }

  for ( cc=0; cc < pa->GetSize(); cc++ )
    {
    char *name = (char *)pa->Lookup(cc);
    if ( name )
      {
      }
    else
      {      
      cout << "At position: " << cc << " there is no name" 
	   << endl;
      error = 1;
      }
    }
  for ( ; pa->GetSize(); )
    {
    int res = pa->Remove(0);
    if ( !res )
      {
      cout << "Remove first element: " << (res ? " ok" : " problem") << endl;
      error = 1;
      }
    }
  if ( pa->GetSize() != 0 )
    {
    cout << "Number of elements left: " << pa->GetSize() << endl;
    error = 1;
    }

  pa->Delete();

  return error;
}

