/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQueue.h
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
// .NAME vtkQueue - a link-list based templated queue

#ifndef __vtkQueue_h
#define __vtkQueue_h

#include "vtkVector.h"

template <class DType> class vtkQueueIterator;

template <class DType>
class vtkQueue : public vtkVector<DType>
{
  friend class vtkQueueIterator<DType>;
public:
  typedef vtkVector<DType> Superclass;

  static vtkQueue<DType> *New();
  virtual const char* GetClassName() const { return "vtkQueue"; }

  // Description:
  // Create the iterator.
  vtkQueueIterator<DType>* NewQueueIterator();

  // Description:
  // Enqueue an item.
  int EnqueueItem(DType a);

  // Description:
  // Dequeue the item.
  int DequeueItem();

  // Description:
  // Get the item to be deueued. This has to be done before calling of
  // DequeueItem
  int GetDequeueItem(DType &a);

  // Description:
  // Display the content of the list.
  virtual void DebugList();

  // Description:
  // Make queue empty.
  void MakeEmpty();

protected:
  vtkQueue();
  virtual ~vtkQueue();

  vtkIdType Start;
  vtkIdType End;

private:
  vtkQueue(const vtkQueue<DType>&); // Not implemented
  void operator=(const vtkQueue<DType>&); // Not implemented
};

#ifdef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION
#include "vtkQueue.txx"
#endif 

#endif
