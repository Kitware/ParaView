/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVMethodInterface.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
// .NAME vtkPVMethodInterface - Every thing needed to define a widget for a method.
// .SECTION Description
// The intent is that subclasses will be createdthe specifically define widgets.


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

class VTK_EXPORT vtkPVMethodInterface : public vtkObject
{
public:
  static vtkPVMethodInterface* New();
  vtkTypeMacro(vtkPVMethodInterface, vtkObject);
  
  // Description:
  // This will be used to label the UI.
  vtkSetStringMacro(VariableName);
  vtkGetStringMacro(VariableName);

  // Description:
  // Most commands can be derived from the names ...
  vtkSetStringMacro(SetCommand);
  vtkGetStringMacro(SetCommand);
  vtkSetStringMacro(GetCommand);
  vtkGetStringMacro(GetCommand);
  
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
  void AddSelectionEntry(int idx, char *label);
  vtkStringList *GetSelectionEntries() {return this->SelectionEntries;}

  // Description:
  // Displays a button that brings up a file selection dialog.
  void SetWidgetTypeToFile() { this->SetWidgetType(VTK_PV_METHOD_WIDGET_FILE);}  
  vtkSetStringMacro(FileExtension);
  vtkGetStringMacro(FileExtension);
  
protected:
  vtkPVMethodInterface();
  ~vtkPVMethodInterface();
  vtkPVMethodInterface(const vtkPVMethodInterface&) {};
  void operator=(const vtkPVMethodInterface&) {};

  char *VariableName;
  char *SetCommand;
  char *GetCommand;

  vtkIdList *ArgumentTypes;
  
  int WidgetType;
  vtkStringList *SelectionEntries;
  
  char *FileExtension;
  
  char *BalloonHelp;
};

#endif
