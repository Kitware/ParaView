/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWHashTable.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include <stdlib.h>

#include "vtkKWHashTable.h"
#include "vtkKWHashTableIterator.h"
#include "vtkObjectFactory.h"


//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWHashTable );

int vtkKWHashTableItem::Count = 0;

vtkKWHashTableItem::vtkKWHashTableItem()
{
  this->Data = 0;
  this->Key = 0;
  this->Next = 0;
  this->Valid = 0;

  this->Count ++;
}

vtkKWHashTableItem::~vtkKWHashTableItem()
{
  this->Count --;
}

void vtkKWHashTableItem::Set( vtkKWHashTableItem *item )
{
  this->Data  = item->Data;
  this->Key   = item->Key;
  this->Next  = item->Next;
  this->Valid = item->Valid;  
}

vtkKWHashTable::vtkKWHashTable()
{
  unsigned int cc;
  this->NumberOfBuckets = KW_HASHTABLE_INITIAL_NUMBER_OF_BUCKETS;
  this->Size = 0;

  this->Table = new vtkKWHashTableItem * [ this->NumberOfBuckets ];
  for ( cc = 0; cc < this->NumberOfBuckets; cc ++ )
    {
    this->Table[cc] = 0;
    }
  this->HashFunction = 0;
}

vtkKWHashTable::~vtkKWHashTable()
{
  this->Clean();
  delete [] this->Table;
  //cout << "Items left: " << vtkKWHashTableItem::Count << endl;
}


// Description:
// Empty the hash table
void
vtkKWHashTable::Clean()
{
  int i = this->NumberOfBuckets-1;
  vtkKWHashTableItem *tmp, *next;

  if ( this->Table != 0 ) 
    {
    while (i --) 
      {
      tmp = this->Table[i];
      next = this->Table[i];
      while ( tmp != 0 )
	{
	next = tmp->Next;
	delete tmp;
	tmp = next;
	}      
      this->Table[i] = 0;
      }
    }

  this->Size = 0;
}

// Description:
// Access an item in the hash table
// Return values:
// non-NULL    The item in question was found; the return value is
//             a pointer to the corresponding record in the hash table.
// NULL        The item was not found in the hash table. Either it just
//             isn't in there, or the hash function returned an error.
void *
vtkKWHashTable::Lookup( unsigned long item )
{
  int slot = item % this->NumberOfBuckets;
  vtkKWHashTableItem *tmp;

  tmp = this->Table[slot];
  //cout << "Looking " << slot << " in " << tmp << endl;

  while (tmp) 
    {
    if (tmp->Key == item) 
      {
      //cout << "We found it" << endl;
      return tmp->Data;
      }
    tmp = tmp->Next;
    }
  return NULL;
}

void *
vtkKWHashTable::Lookup( void *key )
{
  if ( this->HashFunction )
    {
    return this->Lookup( (*this->HashFunction)( key ) );
    }
  return 0;
}

// Description:
// Insert: Insert an item into the hash table
// Return values:
// nonzero     The item was inserted successfully
// zero        The item could not be inserted. Either the function could
//             not allocate the amount of memory necessary to store it,
//             or the hash table already contains an item with the same
//             key, or the hash function returned an error.
// Note:
// If you know for sure that key values are in fact unique identifiers,
// that is, that the calling functions will never try to make the hash
// table contain two items with the same key at the same time, you can
// speed up the function considerably by deleting the first statement.

int
vtkKWHashTable::Insert(unsigned long key, void* data)
{
  int slot;
  vtkKWHashTableItem *tmp;

  /* delete this line to insert items in constant time: */
  if (this->Lookup(key)) return 0;
	
  if ((slot = key % this->NumberOfBuckets) < 0 
      || (tmp = new vtkKWHashTableItem) == NULL) 
    {
    return 0;
    }
  tmp->Data = data;
  tmp->Key = key;
  tmp->Next = this->Table[slot];
  this->Table[slot] = tmp;
  
  this->Size ++;
  return 1;
}

int
vtkKWHashTable::Insert( void *key, void* data )
{
  if ( this->HashFunction )
    {
    return this->Insert( (*this->HashFunction)( key ), data );
    }
  return 0;
}

// Description:
// Remove an item from the hash table
// Return values:
// nonzero     The item was removed successfully.
// zero        The item could not be removed. Either it just wasn't
//             found in the hash table, or the hash function returned
//             an error.
int
vtkKWHashTable::Remove(unsigned long key)
{
  int slot;
  vtkKWHashTableItem **tmp, *kill;
  
  if ((slot = key % this->NumberOfBuckets) < 0) {
  return 0;
  }
  tmp = &this->Table[slot];
  while (*tmp) 
    {
    if ((*tmp)->Key == key) 
      {
      kill = *tmp;
      *tmp = (*tmp)->Next;
      delete kill;
      this->Size --;
      return 1;
      }
    tmp = &((*tmp)->Next);
    }
  return 0;
}

int
vtkKWHashTable::Remove( void *key )
{
  if ( this->HashFunction )
    {
    return this->Remove( (*this->HashFunction)( key ) );
    }
  return 0;
}

unsigned long 
vtkKWHashTable::HashString(const char* s)
{
  if( !s ) 
    {
    return 0;
    }
  unsigned long h = 0; 
  for ( ; *s; ++s)
    {
    h = 5*h + *s;
    }
  return h;
}

vtkKWHashTableIterator *vtkKWHashTable::Iterator()
{
  vtkKWHashTableIterator *it = vtkKWHashTableIterator::New();
  it->SetHashTable( this );
  if ( it->Valid() )
    {
    return it;
    }
  it->Delete();
  return 0;
}
