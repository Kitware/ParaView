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
// .NAME vtkPMIntVectorProperty
// .SECTION Description
//

#ifndef __vtkPMIntVectorProperty_h
#define __vtkPMIntVectorProperty_h

#include "vtkPMVectorProperty.h"
#include "vtkPMVectorPropertyTemplate.h"

#define vtkPMVectorProperty vtkPMVectorPropertyTemplate<int>
class VTK_EXPORT vtkPMIntVectorProperty : public vtkPMVectorProperty
#undef vtkPMVectorProperty
{
public:
  static vtkPMIntVectorProperty* New();
  vtkTypeMacro(vtkPMIntVectorProperty, vtkPMVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPMIntVectorProperty();
  ~vtkPMIntVectorProperty();

private:
  vtkPMIntVectorProperty(const vtkPMIntVectorProperty&); // Not implemented
  void operator=(const vtkPMIntVectorProperty&); // Not implemented
//ETX
};

#endif
