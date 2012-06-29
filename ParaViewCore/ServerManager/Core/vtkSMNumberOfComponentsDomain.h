/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNumberOfComponentsDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMNumberOfComponentsDomain - int range domain based on the number of
// components available in a particular data array.
// .SECTION Description
// vtkSMNumberOfComponentsDomain is used for properties that allow the user to
// choose the component number to process for the choosen array.
// It needs two required properties with following functions:
// * Input -- input property for the filter.
// * ArraySelection -- string vector property used to select the array.
// This domain will not work if either of the required properties is missing.

#ifndef __vtkSMNumberOfComponentsDomain_h
#define __vtkSMNumberOfComponentsDomain_h

#include "vtkSMIntRangeDomain.h"

class vtkSMSourceProxy;
class vtkSMInputArrayDomain;

class VTK_EXPORT vtkSMNumberOfComponentsDomain : public vtkSMIntRangeDomain
{
public:
  static vtkSMNumberOfComponentsDomain* New();
  vtkTypeMacro(vtkSMNumberOfComponentsDomain, vtkSMIntRangeDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Updates the range based on the scalar range of the currently selected
  // array. This requires Input (vtkSMProxyProperty) and ArraySelection 
  // (vtkSMStringVectorProperty) properties. Currently, this uses
  // only the first component of the array.
  virtual void Update(vtkSMProperty* prop);

//BTX
protected:
  vtkSMNumberOfComponentsDomain();
  ~vtkSMNumberOfComponentsDomain();

  // Description:
  // Internal update method doing the actual work.
  void Update(const char* arrayname, 
    vtkSMSourceProxy* sp, vtkSMInputArrayDomain* iad,
    int outputport);

private:
  vtkSMNumberOfComponentsDomain(const vtkSMNumberOfComponentsDomain&); // Not implemented
  void operator=(const vtkSMNumberOfComponentsDomain&); // Not implemented
//ETX
};

#endif

