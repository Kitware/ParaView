/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSourceInterface.h
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
// .NAME vtkPVSourceInterface - Everything PV needs to make a UI for a filter.
// .SECTION Description
// This contains every thing PV needs to create a UI for a VTK source.

#ifndef __vtkPVSourceInterface_h
#define __vtkPVSourceInterface_h

#include "vtkKWObject.h"
#include "vtkPVApplication.h"
#include "vtkPVMethodInterface.h"

#include "vtkCollection.h"
#include "vtkPVWindow.h"
class vtkPVData;


class VTK_EXPORT vtkPVSourceInterface : public vtkKWObject
{
public:
  static vtkPVSourceInterface* New();
  vtkTypeMacro(vtkPVSourceInterface, vtkKWObject);
  
  // Description:
  // The name of the vtk object created.
  vtkSetStringMacro(SourceClassName);
  vtkGetStringMacro(SourceClassName);
  
  // Description:
  // Name (with instance num) displayed in the UI.
  vtkSetStringMacro(RootName);
  vtkGetStringMacro(RootName);
  
  // Description:
  // For now we are supporting only one input and output.
  vtkSetStringMacro(InputClassName);
  vtkGetStringMacro(InputClassName);
  vtkSetStringMacro(OutputClassName);
  vtkGetStringMacro(OutputClassName);
  
  // Description:
  // Set/Get name of input file.
  vtkSetStringMacro(DataFileName);
  vtkGetStringMacro(DataFileName);
  
  // Description:
  // Does this filter operate on scalars?
  vtkBooleanMacro(DefaultScalars, int);
  vtkGetMacro(DefaultScalars, int);
  vtkSetMacro(DefaultScalars, int);

  // Description:
  // Does this filter operate on vectors?
  vtkBooleanMacro(DefaultVectors, int);
  vtkGetMacro(DefaultVectors, int);
  vtkSetMacro(DefaultVectors, int);

  // Description:
  // Access to the method interfaces.
  void AddMethodInterface(vtkPVMethodInterface *methonInt);
  vtkCollection *GetMethodInterfaces() {return this->MethodInterfaces;}
  
  // Description:
  // Where to put any new source composites created.
  // This is here because the callback does not have an argument.
  // No reference counting.
  void SetPVWindow(vtkPVWindow *pvw);
  
  // Description:
  // This method is called by the window to determine if this filter should be
  // added to the filter menu.  Right now, only the class name of the input
  // is checked.  In the future, attributes could be checked as well.
  virtual int GetIsValidInput(vtkPVData *input);
  
  // Decription:
  // This method is called to create another source.
  // Name is usually NULL.  Names are specified for creating glyph sources.
  // In this case, the creation of the source is hidden from the user.
  virtual vtkPVSource *CreateCallback(const char* name, vtkCollection* sourceList);
  vtkPVSource *CreateCallback() 
    {return this->CreateCallback(NULL, this->PVWindow->GetSources());}

  // Description:
  // Save this interface to a file.
  virtual void SaveInTclScript(ofstream *file, const char* sourceName);

  // Description:
  // This flag determines whether a source will make its input invisible or not.
  // By default, this flag is on.
  vtkSetMacro(ReplaceInput, int);
  vtkGetMacro(ReplaceInput, int);
  vtkBooleanMacro(ReplaceInput, int);
  
protected:
  vtkPVSourceInterface();
  ~vtkPVSourceInterface();
  vtkPVSourceInterface(const vtkPVSourceInterface&) {};
  void operator=(const vtkPVSourceInterface&) {};

  char *RootName;
  char *SourceClassName;
  char *InputClassName;
  char *OutputClassName;
  char *DataFileName;
  
  int DefaultScalars;
  int DefaultVectors;
  
  vtkCollection *MethodInterfaces;
  
  // Extra stuff (not specific to interface.)
  vtkPVApplication *GetPVApplication();
  vtkPVWindow *PVWindow;
  int InstanceCount;

  int ReplaceInput;
};

#endif
