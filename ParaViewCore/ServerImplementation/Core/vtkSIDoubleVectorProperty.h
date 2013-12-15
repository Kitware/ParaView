/*=========================================================================

  Program:   ParaView
  Module:    vtkSIDoubleVectorProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSIDoubleVectorProperty
// .SECTION Description
// Vector property that manage double value to be set through a method
// on a vtkObject.

#ifndef __vtkSIDoubleVectorProperty_h
#define __vtkSIDoubleVectorProperty_h

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkSIVectorProperty.h"
#include "vtkSIVectorPropertyTemplate.h" // real superclass

#ifndef __WRAP__
#define vtkSIVectorProperty vtkSIVectorPropertyTemplate<double>
#endif
class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSIDoubleVectorProperty : public vtkSIVectorProperty
#ifndef __WRAP__
#undef vtkSIVectorProperty
#endif
{
public:
  static vtkSIDoubleVectorProperty* New();
  vtkTypeMacro(vtkSIDoubleVectorProperty, vtkSIVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSIDoubleVectorProperty();
  ~vtkSIDoubleVectorProperty();

private:
  vtkSIDoubleVectorProperty(const vtkSIDoubleVectorProperty&); // Not implemented
  void operator=(const vtkSIDoubleVectorProperty&); // Not implemented
//ETX
};

#endif
