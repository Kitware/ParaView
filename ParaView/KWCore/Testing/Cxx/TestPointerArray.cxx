#include "vtkVector.txx"

int CheckName(const char *name, char **names)
{
  int cc;
  for ( cc = 0; names[cc]; cc++ )
    {
    if ( !strncmp(names[cc], name, strlen(names[cc])) )
      {
      return VTK_OK;
      }
    }
  return VTK_ERROR;
}

template<class DType>
int TestList(DType*)
{
  unsigned int cc;
  int error = 0;
  char* names[] = {
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
    "Will",
    0
  };

  char separate[] = "separate";


  DType *strings = DType::New();

  for ( cc =0 ; cc< 10; cc++ )
    {
    if ( strings->AppendItem( names[cc] ) != VTK_OK )
      {
      cout << "Insert failed" << endl;
      error = 1;
      }
    }

  for ( cc=0; cc < 11; cc++ )
    {
    char* name = 0;
    if ( cc < 10 )
      {
      if ( strings->GetItem(cc, name) != VTK_OK )
	{
	cout << "Problem accessing item: " << cc << endl;
	error = 1;
	}
      if ( !name )
	{
	cout << "Name is null" << endl;
	error = 1;
	}
      if ( name && strcmp(name, names[cc]) )
	{
	cout << "Got name but it is not what it should be" << endl;
	error = 1;
	}
      }
    else
      {
      if ( strings->GetItem(cc, name) == VTK_OK )
	{
	cout << "Should not be able to access item: " << cc << endl;
	error = 1;
	}
      }
    }

  for ( cc=1; cc<10; cc+= 2 )
    {
    if ( cc < strings->GetNumberOfItems() )
      {
      if ( strings->RemoveItem(cc) != VTK_OK )
	{
	cout << "Problem removing item: " << cc << endl;
	cout << "Number of elements: " << strings->GetNumberOfItems() << endl;
	error = 1;
	}
      }
    else
      {
      if ( strings->RemoveItem(cc) == VTK_OK )
	{
	cout << "Should not be able to remove item: " << cc << endl;
	cout << "Number of elements: " << strings->GetNumberOfItems() << endl;
	error = 1;
	}
      }
    
    }

  for ( cc=0; cc < 11; cc++ )
    {
    char *name = 0;
    if ( cc < 7 )
      {
      if ( strings->GetItem(cc, name) != VTK_OK )
	{
	cout << "Problem accessing item: " << cc << endl;
	error = 1;
	}
      if ( !name )
	{
	cout << "Name is null" << endl;
	error = 1;
	}
      if ( !name && ::CheckName(name, names) != VTK_OK )
	{
	cout << "Got strange name at position: " << cc << endl;
	error = 1;
	}
      }
    else
      {
      if ( strings->GetItem(cc, name) == VTK_OK )
	{
	cout << "Should not be able to access item: " << cc << endl;
	error = 1;
	}
      }
    }
  
  if ( strings->GetNumberOfItems() != 7 )
    {
    cout << "Number of elements left: " << strings->GetNumberOfItems() << endl;
    error = 1;
    }

  for ( cc =0 ; cc< 100; cc++ )
    {
    if ( strings->PrependItem( separate ) != VTK_OK )
      {
      cout << "Problem prepending item: " << cc << endl;
      error = 1;
      }
    }

  for ( cc=0; cc < strings->GetNumberOfItems(); cc++ )
    {
    char *name = 0;
    if ( strings->GetItem(cc, name) != VTK_OK )
      {
      cout << "Problem accessing item: " << cc << endl;
      error = 1;
      }
    if ( !name )
      {
      cout << "Name is null" << endl;
      error = 1;
      }
    if ( !name && ::CheckName(name, names) != VTK_OK )
      {
      cout << "Got strange name at position: " << cc << endl;
      error = 1;
      }
    }

  for ( ; strings->GetNumberOfItems(); )
    {
    if ( strings->RemoveItem(0) != VTK_OK )
      {
      cout << "Problem remove first element" << endl;
      error = 1;
      }
    }

  if ( strings->GetNumberOfItems() != 0 )
    {
    cout << "Number of elements left: " << strings->GetNumberOfItems() << endl;
    error = 1;
    }

  strings->Delete();

  strings = DType::New();
  strings->SetSize(15);
  for ( cc = 0; cc < 20; cc ++ )
    {
    if ( cc < 15 )
      {
      if ( strings->InsertItem( 0, separate ) != VTK_OK )
	{
	cout << "Problem inserting item: " << cc << endl;
	cout << "Size: " << strings->GetNumberOfItems() << endl;
	error = 1;
	}
      }
    else
      {
      if ( strings->InsertItem( 0, separate ) == VTK_OK )
	{
	cout << "Should not be able to insert item: " << cc << endl;
	cout << "Size: " << strings->GetNumberOfItems() << endl;
	error = 1;
	}
      }
    }
  
  strings->Delete();

  return error;
}

int main()
{
  int res = 0;

  vtkVector<char *> *vv = vtkVector<char *>::New();
  res += TestList(vv);
  vv->Delete();

  // When linked list will be implemented
  // vtkLinkedList<char *> *vl = vtkLinkedList<char *>::New();
  // res += TestList(vl);
  // vl->Delete();

  return res;
}
