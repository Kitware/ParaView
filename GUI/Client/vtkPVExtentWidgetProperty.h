/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExtentWidgetProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVExtentWidgetProperty
// .SECTION Description

#ifndef __vtkPVExtentWidgetProperty_h
#define __vtkPVExtentWidgetProperty_h

#include "vtkPVScalarListWidgetProperty.h"

class VTK_EXPORT vtkPVExtentWidgetProperty : public vtkPVScalarListWidgetProperty
{
public:
  static vtkPVExtentWidgetProperty* New();
  vtkTypeRevisionMacro(vtkPVExtentWidgetProperty, vtkPVScalarListWidgetProperty);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  virtual void SetAnimationTime(float time);
  virtual void SetAnimationTimeInBatch(ofstream *file, float val);
  
protected:
  vtkPVExtentWidgetProperty() {}
  ~vtkPVExtentWidgetProperty() {}
  
private:
  vtkPVExtentWidgetProperty(const vtkPVExtentWidgetProperty&); // Not implemented
  void operator=(const vtkPVExtentWidgetProperty&); // Not implemented
};

#endif
