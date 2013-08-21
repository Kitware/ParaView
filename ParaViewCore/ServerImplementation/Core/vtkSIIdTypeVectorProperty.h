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

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkSIVectorProperty.h"
#include "vtkSIVectorPropertyTemplate.h" // real superclass

#ifndef __WRAP__
#define vtkSIVectorProperty vtkSIVectorPropertyTemplate<vtkIdType, bool>
#endif
class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSIIdTypeVectorProperty : public vtkSIVectorProperty
#ifndef __WRAP__
#undef vtkSIVectorProperty
#endif
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
