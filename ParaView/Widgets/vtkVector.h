/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVector.h
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
// .NAME vtkVector - a dynamic vector

#ifndef __vtkVector_h
#define __vtkVector_h

#include "vtkAbstractList.h"

template <class DType>
class vtkVector : public vtkAbstractList<DType>
{
public:
  vtkContainerTypeMacro(vtkVector<DType>, vtkAbstractList<DType>);
  
  static vtkVector<DType> *New() { return new vtkVector<DType>(); }  
  
  // Description:
  // Append an Item to the end of the vector.
  int AppendItem(DType a);
  
  // Description:
  // Insert an Item to the front of the vector.
  int PrependItem(DType a);
  
  // Description:
  // Insert an Item to the specific location in the vector.
  int InsertItem(unsigned long loc, DType a);
  
  // Description:
  // Sets the Item at the specific location in the list to a new value.
  // It also checks if the item can be set.
  // It returns VTK_OK if successfull.
  virtual int SetItem(unsigned long loc, DType a);

  // Description:
  // Sets the Item at the specific location in the list to a new value.
  // This method does not perform any error checking.
  virtual void SetItemNoCheck(unsigned long loc, DType a);

   // Description:
  // Remove an Item from the vector
  int RemoveItem(unsigned long id);
  
  // Description:
  // Return an item that was previously added to this vector. 
  int GetItem(unsigned long id, DType& ret);
      
  // Description:
  // Find an item in the vector. Return one if it was found, zero if it was
  // not found. The location of the item is returned in res.
  int FindItem(DType a, unsigned long &res);

  // Description:
  // Find an item in the vector using a comparison routine. 
  // Return one if it was found, zero if it was
  // not found. The location of the item is returned in res.
  int FindItem(DType a, vtkAbstractList<DType>::CompareFunction compare, 
	       unsigned long &res);
  
  // Description:
  // Return the number of items currently held in this container. This
  // different from GetSize which is provided for some containers. GetSize
  // will return how many items the container can currently hold.
  virtual unsigned long GetNumberOfItems() { return this->NumberOfItems; }
  
  // Description:
  // Returns the number of items the container can currently hold.
  virtual unsigned long GetSize() { return this->Size; }

  // Description:
  // Removes all items from the container.
  virtual void RemoveAllItems();

  // Description:
  // Set the capacity of the vector.
  // It returns VTK_OK if successfull.
  // If capacity is set, the vector will not resize.
  virtual int SetSize(unsigned long size);

  // Description:
  // Allow or disallow resizing. If resizing is disallowed, when
  // inserting too many elements, it will return VTK_ERROR.
  // Initially allowed.
  void SetResize(int r) { this->Resize = r; }
  void ResizeOn() { this->SetResize(1); }
  void ResizeOff() { this->SetResize(0); }
  int GetResize() { return this->Resize; }

protected:
  vtkVector() {
    this->Array = 0; this->NumberOfItems = 0; this->Size = 0; 
    this->Resize = 1; }
  ~vtkVector() {
    if (this->Array)
      {
      delete [] this->Array;
      }
  }
  unsigned long NumberOfItems;
  unsigned long Size;
  int Resize;
  DType *Array;
};

#ifdef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION
#include "vtkImageIterator.txx"
#endif 

#endif
