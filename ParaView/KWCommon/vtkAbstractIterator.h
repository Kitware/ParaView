/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

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
// .NAME vtkAbstractIterator - is an iterator for vtkContainer subclasses
// .SECTION Description
// vtkAbstractIterator is a superclass of all container iterators.

// .SECTION See Also
// vtkContainer

// .SECTION Caveates
// VTK Iterators are not reliable when adding or deleting elements 
// from the container. Use iterators for traversing only.

#ifndef __vtkAbstractIterator_h
#define __vtkAbstractIterator_h

#include "vtkObjectBase.h"

class vtkContainer;

template<class KeyType, class DataType>
class  vtkAbstractIterator : public vtkObjectBase
{
  friend class vtkContainer;

public:
  // Description:
  // Return the class name as a string.
  virtual const char* GetClassName() const { return "vtkAbstractIterator"; }

  // Description:
  // Retrieve the key from the iterator. For lists, the key is the
  // index of the element.
  // This method returns VTK_OK if key was retrieved correctly.
  //virtual int GetKey(KeyType&) = 0;
  
  // Description:
  // Retrieve the data from the iterator. 
  // This method returns VTK_OK if key was retrieved correctly.
  //virtual int GetData(DataType&) = 0;

  // Description:
  // Retrieve the key and data of the current element.
  // This method returns VTK_OK if key and data were retrieved correctly.
  // virtual int GetKeyAndData(KeyType&, DataType&) = 0;
  
  // Description:
  // Set the container for this iterator.
  void SetContainer(vtkContainer*);

  // Description:
  // Get the associated container.
  vtkContainer *GetContainer() { return this->Container; }

  // Description:
  // Initialize the traversal of the container. 
  // Set the iterator to the "beginning" of the container.
  //virtual void InitTraversal()=0;

  // Description:
  // Check if the iterator is at the end of the container. Return 1
  // for yes, 0 for no.
  //virtual int IsDoneWithTraversal()=0;

  // Description:
  // Increment the iterator to the next location.
  //virtual void GoToNextItem() = 0;

protected:
  vtkAbstractIterator();
  virtual ~vtkAbstractIterator();

  vtkContainer *Container;
  vtkIdType ReferenceCount;

private:
  vtkAbstractIterator(const vtkAbstractIterator&); // Not implemented
  void operator=(const vtkAbstractIterator&); // Not implemented
};

#ifdef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION
#include "vtkAbstractIterator.txx"
#endif

#endif








