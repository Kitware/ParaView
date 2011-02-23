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
// .NAME vtkPMIdTypeVectorProperty
// .SECTION Description
//

#ifndef __vtkPMIdTypeVectorProperty_h
#define __vtkPMIdTypeVectorProperty_h

#include "vtkPMVectorProperty.h"
#include "vtkPMVectorPropertyTemplate.h" // real superclass

#define vtkPMVectorProperty vtkPMVectorPropertyTemplate<vtkIdType, bool>
class VTK_EXPORT vtkPMIdTypeVectorProperty : public vtkPMVectorProperty
#undef vtkPMVectorProperty
{
public:
  static vtkPMIdTypeVectorProperty* New();
  vtkTypeMacro(vtkPMIdTypeVectorProperty, vtkPMVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPMIdTypeVectorProperty();
  ~vtkPMIdTypeVectorProperty();

private:
  vtkPMIdTypeVectorProperty(const vtkPMIdTypeVectorProperty&); // Not implemented
  void operator=(const vtkPMIdTypeVectorProperty&); // Not implemented
//ETX
};

#endif
