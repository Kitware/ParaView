/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVWidget.h
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
// .NAME vtkPVWidget - Intelligent widget for building source interfaces.
// .SECTION Description
// vtkPVWidget is a superclass for widgets that can be used to build
// interfaces for vtkPVSources.  These widgets combine the UI of vtkKWWidgets,
// but can synchronize themselves with vtkObjects.  When the "Accept" method
// is called, this widgets sets the vtkObjects ivars based on the widgets
// value.  When the "Reset" method is called, the vtkObjects state is used to 
// set the widgets value.  When the widget has been modified, and the "Accept"
// method is called,  the widget will save the transition in the trace file.

#ifndef __vtkPVWidget_h
#define __vtkPVWidget_h

#include "vtkKWWidget.h"
#include "vtkStringList.h"
#include "vtkPVSource.h"
#include "vtkPVApplication.h"

class VTK_EXPORT vtkPVWidget : public vtkKWWidget
{
public:
  static vtkPVWidget* New();
  vtkTypeMacro(vtkPVWidget, vtkKWWidget);

  // Description:
  // The methods get called by the source when 
  // the Accept or Reset buttons are pressed.
  virtual void Accept();
  virtual void Reset();

  // Description:
  // This method gets called when the user changes the widgets value,
  // or a script changes the widgets value.  
  virtual void ModifiedCallback();
  
  // Description:
  // Set the vtkPVSource associated with this vtkPVWidget
  void SetPVSource(vtkPVSource *source);
  vtkGetObjectMacro(PVSource, vtkPVSource);

  // Description:
  // Save this widget to a file.  
  virtual void SaveInTclScript(ofstream *file, const char *sourceName);
  
  // Description:
  // With this name, you can get this widget from the PVSource.
  // It is to allow access to this widget from a script.
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);

  // Description:
  // Conveniance method that casts the application to a PV application.
  vtkPVApplication *GetPVApplication() 
    {return vtkPVApplication::SafeDownCast(this->Application);}

protected:
  vtkPVWidget();
  ~vtkPVWidget();
  vtkPVWidget(const vtkPVWidget&) {};
  void operator=(const vtkPVWidget&) {};

  vtkStringList *AcceptCommands;
  vtkStringList *ResetCommands;
  
  char *SetCommand;
  char *GetCommand;
  
  vtkSetStringMacro(SetCommand);
  vtkSetStringMacro(GetCommand);
  
  vtkPVSource *PVSource;

  // This flag indicates that a variable has been defined in the 
  // trace file for this widget.
  int TraceInitialized;
  // This flag indicates that the widget has changed and should be
  // added to the trace file.
  int ModifiedFlag;

  // This name allows access to this widget from a script.
  char *Name;
};

#endif
