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
/**
 * @class   vtkSIDoubleVectorProperty
 *
 * Vector property that manage double value to be set through a method
 * on a vtkObject.
*/

#ifndef vtkSIDoubleVectorProperty_h
#define vtkSIDoubleVectorProperty_h

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
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSIDoubleVectorProperty();
  ~vtkSIDoubleVectorProperty() override;

private:
  vtkSIDoubleVectorProperty(const vtkSIDoubleVectorProperty&) = delete;
  void operator=(const vtkSIDoubleVectorProperty&) = delete;
};

#endif
