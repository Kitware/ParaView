/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSourceCollection.h
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
// .NAME vtkPVSourceCollection - a list of pvSources
// .SECTION Description
// vtkPVSourceCollection represents and provides methods to manipulate a list of
// pvSources.  It's main inteded use is by vtkPVData's.  They are going to keep
// a list of all sources that use the data.

// .SECTION see also
// vtkPVSource vtkCollection 

#ifndef __vtkPVSourceC_h
#define __vtkPVSourceC_h

#include "vtkCollection.h"
class vtkPVSource;

class VTK_EXPORT vtkPVSourceCollection : public vtkCollection
{
public:
  static vtkPVSourceCollection *New();
  vtkTypeMacro(vtkPVSourceCollection,vtkCollection);

  // Description:
  // Add an actor to the list.
  void AddItem(vtkPVSource *a);

  // Description:
  // Get the next actor in the list.
  vtkPVSource *GetNextPVSource();

  // Description:
  // Get the last actor in the list.
  vtkPVSource *GetLastPVSource();

protected:
  vtkPVSourceCollection() {};
  ~vtkPVSourceCollection() {};
  vtkPVSourceCollection(const vtkPVSourceCollection&) {};
  void operator=(const vtkPVSourceCollection&) {};
    

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };
};

inline void vtkPVSourceCollection::AddItem(vtkPVSource *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

inline vtkPVSource *vtkPVSourceCollection::GetNextPVSource() 
{ 
  return (vtkPVSource *)(this->GetNextItemAsObject());
}

inline vtkPVSource *vtkPVSourceCollection::GetLastPVSource() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return (vtkPVSource *)(this->Bottom->Item);
    }
}


#endif





