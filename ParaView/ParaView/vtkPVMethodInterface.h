/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMethodInterface.h
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
// .NAME vtkPVMethodInterface - Every thing needed to define a widget for a method.
// .SECTION Description
// The intent is that subclasses will be created that specifically define widgets.


#ifndef __vtkPVMethodInterface_h
#define __vtkPVMethodInterface_h

#include "vtkObject.h"
#include "vtkIdList.h"

class vtkStringList;


#define VTK_STRING 13

#define VTK_PV_METHOD_WIDGET_ENTRY      0
#define VTK_PV_METHOD_WIDGET_TOGGLE     1
#define VTK_PV_METHOD_WIDGET_SELECTION  2
#define VTK_PV_METHOD_WIDGET_FILE       3
#define VTK_PV_METHOD_WIDGET_EXTENT     4

class VTK_EXPORT vtkPVMethodInterface : public vtkObject
{
public:
  static vtkPVMethodInterface* New();
  vtkTypeMacro(vtkPVMethodInterface, vtkObject);
  
  // Description:
  // This will be used to label the UI.
  vtkSetStringMacro(Label);
  vtkGetStringMacro(Label);

  // Description:
  // The name of the variable used for scripting.
  vtkSetStringMacro(VariableName);
  vtkGetStringMacro(VariableName);
  
  // Description:
  // This specifies the type of widget to use.
  // I have not descided whether we should have subclasses that specifies
  // the widget and its parameters.  For now, it is just a mode in this
  // geneneric interface.  Default is just an entry box.
  void SetWidgetType(int type);
  vtkGetMacro(WidgetType,int);

  // Description:
  // Set balloon help for this widget.
  vtkSetStringMacro(BalloonHelp);
  vtkGetStringMacro(BalloonHelp);
  
  // Description:
  // Add the argument types one by one.
  // Only the entry widget supports multiple arguments.  
  // Toggle and Selection should both have one integer argument.
  void SetWidgetTypeToEntry() { this->SetWidgetType(VTK_PV_METHOD_WIDGET_ENTRY);}
  void AddArgumentType(int type);
  void AddFloatArgument() {this->AddArgumentType(VTK_FLOAT);}
  void AddIntegerArgument() {this->AddArgumentType(VTK_INT);}
  void AddStringArgument() {this->AddArgumentType(VTK_STRING);}
  int GetNumberOfArguments() {return this->ArgumentTypes->GetNumberOfIds();}
  int GetArgumentType(int i) {return this->ArgumentTypes->GetId(i);}  

  // Discription:
  // Toggle an integer ivar.  This is the only call required.
  void SetWidgetTypeToToggle() { this->SetWidgetType(VTK_PV_METHOD_WIDGET_TOGGLE);}  
  
  // Description:
  // For a selection widget.  Add the string options.
  void SetWidgetTypeToSelection() { this->SetWidgetType(VTK_PV_METHOD_WIDGET_SELECTION);}
  void AddSelectionEntry(int idx, const char *label);
  vtkStringList *GetSelectionEntries() {return this->SelectionEntries;}

  // Description:
  // Displays a button that brings up a file selection dialog.
  void SetWidgetTypeToFile() { this->SetWidgetType(VTK_PV_METHOD_WIDGET_FILE);}  
  // Description:
  // Displays an extent entry widget.
  void SetWidgetTypeToExtent() { this->SetWidgetType(VTK_PV_METHOD_WIDGET_EXTENT); }
  
  vtkSetStringMacro(FileExtension);
  vtkGetStringMacro(FileExtension);
  
protected:
  vtkPVMethodInterface();
  ~vtkPVMethodInterface();
  vtkPVMethodInterface(const vtkPVMethodInterface&) {};
  void operator=(const vtkPVMethodInterface&) {};

  char *Label;
  char *VariableName;

  vtkIdList *ArgumentTypes;
  
  int WidgetType;
  vtkStringList *SelectionEntries;
  
  char *FileExtension;
  
  char *BalloonHelp;
};

#endif
