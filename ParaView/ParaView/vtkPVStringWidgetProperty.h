/*=========================================================================

  Program:   ParaView
  Module:    vtkPVStringWidgetProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVStringWidgetProperty
// .SECTION Description

#ifndef __vtkPVStringWidgetProperty_h
#define __vtkPVStringWidgetProperty_h

#include "vtkPVWidgetProperty.h"

#include "vtkPVSelectWidget.h" // Needed for vtkPVSelectWidget::ElementType
#include "vtkClientServerID.h" // Needed for ObjectID

class VTK_EXPORT vtkPVStringWidgetProperty : public vtkPVWidgetProperty
{
public:
  static vtkPVStringWidgetProperty* New();
  vtkTypeRevisionMacro(vtkPVStringWidgetProperty, vtkPVWidgetProperty);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  vtkSetStringMacro(String);
  vtkGetStringMacro(String);
  
  virtual void AcceptInternal();
  //BTX
  vtkSetMacro(ObjectID, vtkClientServerID);
  //ETX
  vtkSetStringMacro(VTKCommand);
  //BTX
  void SetStringType(vtkPVSelectWidget::ElementTypes);
  //ETX
protected:
  vtkPVStringWidgetProperty();
  ~vtkPVStringWidgetProperty();

  vtkClientServerID ObjectID;
  char *String;
  char *VTKCommand;
  //BTX
  vtkPVSelectWidget::ElementTypes ElementType;
  //ETX
private:
  vtkPVStringWidgetProperty(const vtkPVStringWidgetProperty&); // Not implemented
  void operator=(const vtkPVStringWidgetProperty&); // Not implemented
};

#endif
