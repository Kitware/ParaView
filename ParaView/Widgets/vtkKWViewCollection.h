/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWViewCollection.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
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
// .NAME vtkKWViewCollection - a collection of views
// .SECTION Description
// vtkKWViewCollection represents and provides methods to manipulate a list of
// views. The list is unsorted and duplicate entries are not prevented.

#ifndef __vtkKWViewC_h
#define __vtkKWViewC_h

#include "vtkCollection.h"
#include "vtkKWView.h"

class VTK_EXPORT vtkKWViewCollection : public vtkCollection
{
public:
  static vtkKWViewCollection *New();
  vtkTypeMacro(vtkKWViewCollection,vtkCollection);

  // Description:
  // Add an KWView to the list.
  void AddItem(vtkKWView *a);

  // Description:
  // Remove an KWView from the list.
  void RemoveItem(vtkKWView *a);

  // Description:
  // Determine whether a particular KWView is present. Returns its position
  // in the list.
  int IsItemPresent(vtkKWView *a);

  // Description:
  // Get the next KWView in the list.
  vtkKWView *GetNextKWView();

  // Description:
  // Get the last KWView in the list.
  vtkKWView *GetLastKWView();

protected:
  vtkKWViewCollection() {};
  ~vtkKWViewCollection() {};

private:
  vtkKWViewCollection(const vtkKWViewCollection&); // Not implemented
  void operator=(const vtkKWViewCollection&); // Not implemented
};

inline void vtkKWViewCollection::AddItem(vtkKWView *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

inline void vtkKWViewCollection::RemoveItem(vtkKWView *a) 
{
  this->vtkCollection::RemoveItem((vtkObject *)a);
}

inline int vtkKWViewCollection::IsItemPresent(vtkKWView *a) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)a);
}

inline vtkKWView *vtkKWViewCollection::GetNextKWView() 
{ 
  return vtkKWView::SafeDownCast(this->GetNextItemAsObject());
}

inline vtkKWView *vtkKWViewCollection::GetLastKWView() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return vtkKWView::SafeDownCast(this->Bottom->Item);
    }
}

#endif





