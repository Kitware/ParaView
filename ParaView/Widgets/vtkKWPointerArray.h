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
  // Set and get the size real size of the array
  vtkGetMacro( ArraySize, unsigned int );

  // Description:
  // Get the number of entries stored in the array
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
  // Append: appends an item to the end of array
  // Return values:
  // nonzero     The item was appended successfully
  // zero        The item could not be appended.
  int Append( void * );

  int Prepend( void * );

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
  vtkSetMacro( ArraySize, unsigned int );

  vtkKWPointerArray();
  ~vtkKWPointerArray();
  vtkKWPointerArray(const vtkKWPointerArray&);  // Not implemented.
  void operator=(const vtkKWPointerArray&);  // Not implemented.

  // Description:
  // Make sure there is enough space for the array.
  // If argument skip is one, the data will be shifted up
  // by one.
  int AllocateSpace(int skip);

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
