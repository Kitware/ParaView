/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWPointerArray.h
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
// .NAME vtkKWPointerArray - hash table to be used in VTK
// .SECTION Description
// vtkKWPointerArray is a simple implementation of the dynamic array

#ifndef __vtkKWPointerArray_h
#define __vtkKWPointerArray_h

#include "vtkKWObject.h"

#define KW_ARRAY_MIN_ELEMENTS 2

class VTK_EXPORT vtkKWPointerArray : public vtkKWObject
{
public: 
  static vtkKWPointerArray *New();
  vtkTypeMacro(vtkKWPointerArray,vtkObject);

  // Description:
  // Set and get the number of buckets.
  // NOTE: when setting the number of buckets, the code is missing
  // that will reorganize the hash elements in the buckets.
  vtkSetMacro( ArraySize, unsigned int );
  vtkGetMacro( ArraySize, unsigned int );

  // Description:
  // Get the number of entries stored in the hash table
  vtkGetMacro( Size, unsigned int );
  
  // Description:
  // Access an item in the hash table
  // Return values:
  // non-NULL    The item in question was found; the return value is
  //             a pointer to the corresponding record in the hash table.
  // NULL        The item was not found in the hash table. Either it just
  //             isn't in there, or the hash function returned an error.
  void *Lookup( unsigned long );

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
  int Append( void * );

  // Description:
  // Remove an item from the hash table
  // Return values:
  // nonzero     The item was removed successfully.
  // zero        The item could not be removed. Either it just wasn't
  //             found in the hash table, or the hash function returned
  //             an error.
  int Remove( unsigned long );

  // Description:
  // Empty the hash table
  void Clean();

private:
  vtkKWPointerArray();
  ~vtkKWPointerArray();
  vtkKWPointerArray(const vtkKWPointerArray&);  // Not implemented.
  void operator=(const vtkKWPointerArray&);  // Not implemented.

  // Description:
  // Table of array elements
  void **Table;
  
  // Description:
  // Number of elements currently stored in the hash table.
  unsigned int Size;

  // Description:
  // Number of buckets to store data. Should be some large 
  // prime number.
  unsigned int ArraySize;
};


#endif // __vtkKWPointerArray_h
