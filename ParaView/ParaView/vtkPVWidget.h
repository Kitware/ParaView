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
  // The point of a PV widget is that it is an interface for
  // some objects state/ivars.  This is one way the object/variable
  // can be specified. Subclasses may have seperate or addition
  // variables for specifying the relationship.
  void SetObjectVariable(const char *objectTclName, const char *var);

  // Description:
  // This commands is an optional action that will be called when
  // the widget is modified.
  void SetModifiedCommand(const char* cmdObject, const char* methodAndArgs);

  // Description:
  // The methods get called by the source when 
  // the Accept or Reset buttons are pressed.
  virtual void Accept();
  virtual void Reset();

  // Description:
  // This method gets called when the user changes the widgets value,
  // or a script changes the widgets value.  Ideally, this method should 
  // be protected.
  virtual void ModifiedCallback();

  // Description:
  // Access to the flag that indicates whether the widgets
  // has been modified and is out of sync with its VTK object.
  vtkGetMacro(ModifiedFlag,int);
  
  // Description:
  // With this name, you can get this widget from the PVSource.
  // It is to allow access to this widget from a script.
  // In most widgets, it is the same as the label.
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);

  // Description:
  // Conveniance method that casts the application to a PV application.
  vtkPVApplication *GetPVApplication() 
    {return vtkPVApplication::SafeDownCast(this->Application);}

  // Description:
  // The owner of this object is responsible for setting the variable
  // in the trace file that refers to this object.  This flag should
  // be set to indicate whether the variable has been set in the trace.
  // The variable is just the array pv index by the objects Tcl name.
  vtkSetMacro(TraceVariableInitialized,int);
  vtkGetMacro(TraceVariableInitialized,int);

  // Description:
  // Save this widget to a file.  
  virtual void SaveInTclScript(ofstream *file, const char *sourceName);  

protected:
  vtkPVWidget();
  ~vtkPVWidget();
  vtkPVWidget(const vtkPVWidget&) {};
  void operator=(const vtkPVWidget&) {};

  vtkStringList *AcceptCommands;
  vtkStringList *ResetCommands;
  
  char *ObjectTclName;
  char *VariableName;
  vtkSetStringMacro(ObjectTclName);
  vtkSetStringMacro(VariableName);

  char *ModifiedCommandObjectTclName;
  char *ModifiedCommandMethod;
  vtkSetStringMacro(ModifiedCommandObjectTclName);
  vtkSetStringMacro(ModifiedCommandMethod);
  
  // This flag indicates that a variable has been defined in the 
  // trace file for this widget.
  int TraceVariableInitialized;
  // This flag indicates that the widget has changed and should be
  // added to the trace file.
  int ModifiedFlag;

  // This name allows access to this widget from a script.
  // In most widgets, it is the same as the label.
  char *Name;
};

#endif
