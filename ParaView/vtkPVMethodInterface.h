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

#define VTK_STRING 13

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
  // Add the argument types one by one.
  void AddArgumentType(int type);
  void AddFloatArgument() {this->AddArgumentType(VTK_FLOAT);}
  void AddIntegerArgument() {this->AddArgumentType(VTK_INT);}
  void AddStringArgument() {this->AddArgumentType(VTK_STRING);}
  
  // Description:
  // Accessing the arguments.
  int GetNumberOfArguments() {return this->ArgumentTypes->GetNumberOfIds();}
  int GetArgumentType(int i) {return this->ArgumentTypes->GetId(i);}
  
protected:
  vtkPVMethodInterface();
  ~vtkPVMethodInterface();
  vtkPVMethodInterface(const vtkPVMethodInterface&) {};
  void operator=(const vtkPVMethodInterface&) {};

  char *VariableName;
  char *SetCommand;
  char *GetCommand;

  vtkIdList *ArgumentTypes;
};

#endif
