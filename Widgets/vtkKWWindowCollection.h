/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWWindowCollection.h
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
// .NAME vtkKWWindowCollection - a collection of windows.
// .SECTION Description
// vtkKWWindowCollection represents and provides methods to manipulate a list 
// of windows. The list is unsorted and duplicate entries are not prevented.

// .SECTION See Also
// vtkKWWindow

#ifndef __vtkKWWindowC_h
#define __vtkKWWindowC_h

#include "vtkCollection.h"
#include "vtkKWWindow.h"

class VTK_EXPORT vtkKWWindowCollection : public vtkCollection
{
public:
  static vtkKWWindowCollection *New();
  vtkTypeMacro(vtkKWWindowCollection,vtkCollection);

  // Description:
  // Add an KWWindow to the list.
  void AddItem(vtkKWWindow *a);

  // Description:
  // Remove an KWWindow from the list.
  void RemoveItem(vtkKWWindow *a);

  // Description:
  // Determine whether a particular KWWindow is present. Returns its position
  // in the list.
  int IsItemPresent(vtkKWWindow *a);

  // Description:
  // Get the next KWWindow in the list.
  vtkKWWindow *GetNextKWWindow();

  // Description:
  // Get the last KWWindow in the list.
  vtkKWWindow *GetLastKWWindow();

protected:
  vtkKWWindowCollection() {};
  ~vtkKWWindowCollection() {};
  vtkKWWindowCollection(const vtkKWWindowCollection&) {};
  void operator=(const vtkKWWindowCollection&) {};
};

inline void vtkKWWindowCollection::AddItem(vtkKWWindow *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

inline void vtkKWWindowCollection::RemoveItem(vtkKWWindow *a) 
{
  this->vtkCollection::RemoveItem((vtkObject *)a);
}

inline int vtkKWWindowCollection::IsItemPresent(vtkKWWindow *a) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)a);
}

inline vtkKWWindow *vtkKWWindowCollection::GetNextKWWindow() 
{ 
  return vtkKWWindow::SafeDownCast(this->GetNextItemAsObject());
}

inline vtkKWWindow *vtkKWWindowCollection::GetLastKWWindow() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return vtkKWWindow::SafeDownCast(this->Bottom->Item);
    }
}

#endif





