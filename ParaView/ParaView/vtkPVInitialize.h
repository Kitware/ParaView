/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInitialize.h
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
// .NAME vtkPVInitialize - A super class for filter objects.
// .SECTION Description
// This is a parallel object.  It needs to be cloned to work correctly.  
// After cloning, the parallel nature of the object is transparent.
// This class should probably be merged with vtkPVComposite.
// Note when there are multiple outputs, a dummy pvsource has to
// be attached to each of those. This way, the user can add modules
// after each output.


#ifndef __vtkPVInitialize_h
#define __vtkPVInitialize_h

#include "vtkKWObject.h"

class vtkPVWindow;

class VTK_EXPORT vtkPVInitialize : public vtkKWObject
{
public:
  static vtkPVInitialize* New();
  vtkTypeRevisionMacro(vtkPVInitialize,vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void Initialize(vtkPVWindow*);

protected:
  vtkPVInitialize() 
    {
    this->StandardFiltersString = 0;
    this->StandardManipulatorsString = 0;
    this->StandardReadersString = 0;
    this->StandardSourcesString = 0;
    this->StandardWritersString = 0;
    }
  ~vtkPVInitialize() {}

  char* GetStandardFiltersInterfaces();
  char* GetStandardManipulatorsInterfaces();
  char* GetStandardReadersInterfaces();
  char* GetStandardSourcesInterfaces();
  char* GetStandardWritersInterfaces();

  char* StandardFiltersString;
  char* StandardManipulatorsString;
  char* StandardReadersString;
  char* StandardSourcesString;
  char* StandardWritersString;

  vtkSetStringMacro(StandardFiltersString);
  vtkSetStringMacro(StandardManipulatorsString);
  vtkSetStringMacro(StandardReadersString);
  vtkSetStringMacro(StandardSourcesString);
  vtkSetStringMacro(StandardWritersString);

private:
  vtkPVInitialize(const vtkPVInitialize&); // Not implemented
  void operator=(const vtkPVInitialize&); // Not implemented
};

#endif
