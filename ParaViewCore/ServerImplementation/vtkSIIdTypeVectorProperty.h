/*=========================================================================

  Program:   ParaView
  Module:    vtkSIIdTypeVectorProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSIIdTypeVectorProperty
// .SECTION Description
// IdType ServerSide Property use to set IdType array as method parameter.

#ifndef __vtkSIIdTypeVectorProperty_h
#define __vtkSIIdTypeVectorProperty_h

#include "vtkSIVectorProperty.h"
#include "vtkSIVectorPropertyTemplate.h" // real superclass

#define vtkSIVectorProperty vtkSIVectorPropertyTemplate<vtkIdType, bool>
class VTK_EXPORT vtkSIIdTypeVectorProperty : public vtkSIVectorProperty
#undef vtkSIVectorProperty
{
public:
  static vtkSIIdTypeVectorProperty* New();
  vtkTypeMacro(vtkSIIdTypeVectorProperty, vtkSIVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSIIdTypeVectorProperty();
  ~vtkSIIdTypeVectorProperty();

private:
  vtkSIIdTypeVectorProperty(const vtkSIIdTypeVectorProperty&); // Not implemented
  void operator=(const vtkSIIdTypeVectorProperty&); // Not implemented
//ETX
};

#endif
