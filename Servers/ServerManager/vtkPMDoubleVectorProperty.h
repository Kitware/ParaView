/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPMDoubleVectorProperty
// .SECTION Description
//

#ifndef __vtkPMDoubleVectorProperty_h
#define __vtkPMDoubleVectorProperty_h

#include "vtkPMVectorProperty.h"
#include "vtkPMVectorPropertyTemplate.h"

#define vtkPMVectorProperty vtkPMVectorPropertyTemplate<double>
class VTK_EXPORT vtkPMDoubleVectorProperty : public vtkPMVectorProperty
#undef vtkPMVectorProperty
{
public:
  static vtkPMDoubleVectorProperty* New();
  vtkTypeMacro(vtkPMDoubleVectorProperty, vtkPMVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPMDoubleVectorProperty();
  ~vtkPMDoubleVectorProperty();

private:
  vtkPMDoubleVectorProperty(const vtkPMDoubleVectorProperty&); // Not implemented
  void operator=(const vtkPMDoubleVectorProperty&); // Not implemented
//ETX
};

#endif
