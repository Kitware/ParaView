#include "vtkVector.txx"
#include "vtkLinkedList.txx"

int CheckName(const char *name, char **names)
{
  int cc;
  if ( !name )
    {
    cout << "Trying to compare with empty name" << endl;
    return VTK_ERROR;
    }
  for ( cc = 0; names[cc]; cc++ )
    {
    if ( !strncmp(names[cc], name, strlen(names[cc])) )
      {
      return VTK_OK;
      }
    }
  return VTK_ERROR;
}

#define C_ERROR(c) cout << "Container: " << c->GetClassName() << " "

template<class DType>
int TestList(DType*)
{
  vtkIdType cc;
  int error = 0;
  const char* names[] = {
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

  const char separate[] = "separate";


  DType *strings = DType::New();

  for ( cc =0 ; cc< 10; cc++ )
    {
    if ( strings->AppendItem( names[cc] ) != VTK_OK )
      {
      C_ERROR(strings) << "Append failed" << endl;
      error = 1;
      }
    }
  //strings->DebugList();

  for ( cc=0; cc < 13; cc++ )
    {
    char* name = 0;
    if ( cc < 10 )
      {
      if ( strings->GetItem(cc, name) != VTK_OK )
	{
	C_ERROR(strings) << "Problem accessing item: " << cc << endl;
	error = 1;
	}
      if ( !name )
	{
	C_ERROR(strings) << "Name is null" << endl;
	error = 1;
	}
      if ( name && strcmp(name, names[cc]) )
	{
	C_ERROR(strings) << "Got name but it is not what it should be" 
			 << endl;
	error = 1;
	}
      }
    else
      {
      if ( strings->GetItem(cc, name) == VTK_OK )
	{
	C_ERROR(strings) << "Should not be able to access item: " 
			 << cc << endl;
	C_ERROR(strings) << "Item: " << name << endl;
	error = 1;
	}
      }
    }
  //strings->DebugList();

  for ( cc=1; cc<10; cc+= 2 )
    {
    if ( cc < strings->GetNumberOfItems() )
      {
      if ( strings->RemoveItem(cc) != VTK_OK )
	{
	C_ERROR(strings) << "Problem removing item: " << cc << endl;
	C_ERROR(strings) << "Number of elements: " 
			 << strings->GetNumberOfItems() 
			 << endl;
	error = 1;
	}
      }
    else
      {
      if ( strings->RemoveItem(cc) == VTK_OK )
	{
	C_ERROR(strings) << "Should not be able to remove item: " 
			 << cc << endl;
	C_ERROR(strings) << "Number of elements: " 
			 << strings->GetNumberOfItems() 
	     << endl;
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
	C_ERROR(strings) << "Problem accessing item: " << cc << endl;
	error = 1;
	}
      if ( !name )
	{
	C_ERROR(strings) << "Name is null" << endl;
	error = 1;
	}
      if ( !name || ::CheckName(name, names) != VTK_OK )
	{
	C_ERROR(strings) << "Got strange name at position: " 
			 << cc << endl;
	error = 1;
	}
      }
    else
      {
      if ( strings->GetItem(cc, name) == VTK_OK )
	{
	C_ERROR(strings) << "Should not be able to access item: " 
			 << cc << endl;
	C_ERROR(strings) << "Item: " << name << endl;
	error = 1;
	}
      }
    }
  
  if ( strings->GetNumberOfItems() != 7 )
    {
    C_ERROR(strings) << "Number of elements left: " 
		     << strings->GetNumberOfItems() << endl;
    error = 1;
    }

  for ( cc =0 ; cc< 100; cc++ )
    {
    if ( strings->PrependItem( separate ) != VTK_OK )
      {
      C_ERROR(strings) << "Problem prepending item: " << cc << endl;
      error = 1;
      }
    }

  for ( cc=0; cc < strings->GetNumberOfItems(); cc++ )
    {
    char *name = 0;
    if ( strings->GetItem(cc, name) != VTK_OK )
      {
      C_ERROR(strings) << "Problem accessing item: " << cc << endl;
      error = 1;
      }
    if ( !name )
      {
      C_ERROR(strings) << "Name is null" << endl;
      error = 1;
      }
    if ( !name && ::CheckName(name, names) != VTK_OK )
      {
      C_ERROR(strings) << "Got strange name at position: " << cc << endl;
      error = 1;
      }
    }


  // Try the iterator
  typename DType::IteratorType *it = strings->NewIterator();
  //cout << "Try iterator" << endl;
  while ( it->IsDoneWithTraversal() != VTK_OK )
    {
    char* str = 0;
    vtkIdType idx = 0;
    if ( it->GetData(str) != VTK_OK )
      {
      C_ERROR(strings) << "Problem accessing data from iterator" << endl;
      error =1;
      }
    if ( it->GetKey(idx) != VTK_OK )
      {
      C_ERROR(strings) << "Problem accessing data from iterator" << endl;
      error =1;     
      }
    //cout << "Item: " << idx << " = " << str << endl;
    it->GoToNextItem();
    }
  it->Delete();

  for ( ; strings->GetNumberOfItems(); )
    {
    if ( strings->RemoveItem(0) != VTK_OK )
      {
      C_ERROR(strings) << "Problem remove first element" << endl;
      error = 1;
      }
    }

  if ( strings->GetNumberOfItems() != 0 )
    {
    C_ERROR(strings) << "Number of elements left: " 
		     << strings->GetNumberOfItems() << endl;
    error = 1;
    }

  strings->Delete();

  strings = DType::New();
  vtkIdType maxsize = 0;
  if ( strings->SetSize(15) == VTK_OK )
    {
    maxsize = 15;
    }
  for ( cc = 0; cc < 20; cc ++ )
    {
    if ( !maxsize || cc < maxsize )
      {
      if ( strings->InsertItem( (cc)?(cc-1):0, separate ) != VTK_OK )
	{
	C_ERROR(strings) << "Problem inserting item: " << cc << endl;
	C_ERROR(strings) << "Size: " << strings->GetNumberOfItems() << endl;
	error = 1;
	}
      }
    else
      {
      if ( strings->InsertItem( (cc)?(cc-1):0, separate ) == VTK_OK )
	{
	C_ERROR(strings) << "Should not be able to insert item: " 
			 << cc << endl;
	C_ERROR(strings) << "Size: " << strings->GetNumberOfItems() << endl;
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

  //cout << "Vector: " << endl;
  vtkVector<char *> *vv = vtkVector<char *>::New();
  res += TestList(vv);
  vv->Delete();

  //cout << "Linked List: " << endl;
  vtkLinkedList<char *> *vl = vtkLinkedList<char *>::New();
  res += TestList(vl);
  vl->Delete();

  return res;
}
