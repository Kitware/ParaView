/*=========================================================================

  Program:   ParaView
  Module:    vtkPVContourWidgetProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVContourWidgetProperty
// .SECTION Description

#ifndef __vtkPVContourWidgetProperty_h
#define __vtkPVContourWidgetProperty_h

#include "vtkPVScalarListWidgetProperty.h"

class VTK_EXPORT vtkPVContourWidgetProperty : public vtkPVScalarListWidgetProperty
{
public:
  static vtkPVContourWidgetProperty* New();
  vtkTypeRevisionMacro(vtkPVContourWidgetProperty, vtkPVScalarListWidgetProperty);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  virtual void SetAnimationTime(float time);
  virtual void SetAnimationTimeInBatch(ofstream *file, float val);

  virtual void AcceptInternal();
  
protected:
  vtkPVContourWidgetProperty() {}
  ~vtkPVContourWidgetProperty() {}
  
private:
  vtkPVContourWidgetProperty(const vtkPVContourWidgetProperty&); // Not implemented
  void operator=(const vtkPVContourWidgetProperty&); // Not implemented
};

#endif
