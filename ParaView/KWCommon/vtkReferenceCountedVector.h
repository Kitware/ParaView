/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReferenceCountedVector.h
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
// .NAME vtkReferenceCountedVector - a dynamic vector with reference counting
//
// .SECTION Description
// This is a vector class with reference counting. It makes some
// assumptions about the type it is storing. Each element is a pointer 
// to a type that has methods Register(void*) and Unregister(void*).

#ifndef __vtkReferenceCountedVector_h
#define __vtkReferenceCountedVector_h

#include "vtkVector.h"

template <class DType>
class vtkReferenceCountedVector : public vtkVector<DType>
{
public:
  vtkContainerTypeMacro(vtkReferenceCountedVector<DType>, 
			vtkVector<DType>);
  
  static vtkReferenceCountedVector<DType> *New() 
    { return new vtkReferenceCountedVector<DType>(); }  
  
  // Description:
  // Append an Item to the end of the vector.
  virtual int AppendItem(DType a);
  
  // Description:
  // Insert an Item to the specific location in the vector.
  virtual int InsertItem(vtkIdType loc, DType a);
  
  // Description:
  // Sets the Item at the specific location in the list to a new value.
  // This method does not perform any error checking.
  virtual void SetItemNoCheck(vtkIdType loc, DType a);

   // Description:
  // Remove an Item from the vector
  virtual int RemoveItem(vtkIdType id);
  
  // Description:
  // Removes all items from the container.
  virtual void RemoveAllItems();

  // Description:
  // Since we know that the storage type is a pointer, we can use
  // this knowledge to have easier acces for its members. This
  // method returns either NULL or the object.
  DType GetItem(vtkIdType id);

protected:
  vtkReferenceCountedVector() {}
  virtual ~vtkReferenceCountedVector();
};

#ifdef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION
#include "vtkReferenceCountedVector.txx"
#endif 

#endif
