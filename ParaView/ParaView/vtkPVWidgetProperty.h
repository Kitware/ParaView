/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWidgetProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVWidgetProperty
// .SECTION Description

#ifndef __vtkPVWidgetProperty_h
#define __vtkPVWidgetProperty_h

#include "vtkObject.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID
class vtkPVWidget;

class VTK_EXPORT vtkPVWidgetProperty : public vtkObject
{
public:
  static vtkPVWidgetProperty* New();
  vtkTypeRevisionMacro(vtkPVWidgetProperty, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void SetWidget(vtkPVWidget *widget);
  vtkGetObjectMacro(Widget, vtkPVWidget);

  virtual void Reset();
  virtual void Accept();
  virtual void AcceptInternal() {}

  virtual void SetAnimationTime(float) {}
  // BTX
  vtkSetMacro(VTKSourceID,vtkClientServerID);
  // ETX
protected:
  vtkPVWidgetProperty();
  ~vtkPVWidgetProperty();

  vtkPVWidget *Widget;
  vtkClientServerID VTKSourceID;
  
private:
  vtkPVWidgetProperty(const vtkPVWidgetProperty&); // Not implemented
  void operator=(const vtkPVWidgetProperty&); // Not implemented
};

#endif
