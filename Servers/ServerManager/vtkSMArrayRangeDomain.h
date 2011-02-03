/*=========================================================================

  Program:   ParaView
  Module:    vtkSMArrayRangeDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMArrayRangeDomain - double range domain based on array range
// .SECTION Description
// vtkSMArrayRangeDomain is a sub-class of vtkSMDoubleRangeDomain. In it's
// Update(), it sets min/max values based on the range of an input array.
// It requires Input (vtkSMProxyProperty) and ArraySelection
// (vtkSMStringVectorProperty) properties.
// .SECTION See Also
// vtkSMDoubleRangeDomain vtkSMProxyProperty vtkSMStringVectorProperty

#ifndef __vtkSMArrayRangeDomain_h
#define __vtkSMArrayRangeDomain_h

#include "vtkSMDoubleRangeDomain.h"

class vtkSMProxyProperty;
class vtkSMSourceProxy;
class vtkSMInputArrayDomain;
class vtkPVDataSetAttributesInformation;

class VTK_EXPORT vtkSMArrayRangeDomain : public vtkSMDoubleRangeDomain
{
public:
  static vtkSMArrayRangeDomain* New();
  vtkTypeMacro(vtkSMArrayRangeDomain, vtkSMDoubleRangeDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Updates the range based on the scalar range of the currently selected
  // array. This requires Input (vtkSMProxyProperty) and ArraySelection
  // (vtkSMStringVectorProperty) properties. Currently, this uses
  // only the first component of the array.
  virtual void Update(vtkSMProperty* prop);

  // Description:
  // A vtkSMProperty is often defined with a default value in the
  // XML itself. However, many times, the default value must be determined
  // at run time. To facilitate this, domains can override this method
  // to compute and set the default value for the property.
  // Note that unlike the compile-time default values, the
  // application must explicitly call this method to initialize the
  // property.
  virtual int SetDefaultValues(vtkSMProperty*);
protected:
  vtkSMArrayRangeDomain();
  ~vtkSMArrayRangeDomain();

private:
  void Update(const char* arrayName,
              vtkSMProxyProperty* ip,
              vtkSMSourceProxy* sp,
              int outputport);
  void Update(const char* arrayName,
              vtkSMSourceProxy* sp,
              vtkSMInputArrayDomain* iad,
              int outputport);
  bool SetArrayRange(vtkPVDataSetAttributesInformation* info,
                     const char* arrayName);
  bool SetArrayRangeForAutoConvertProperty(
          vtkPVDataSetAttributesInformation* info,
          const char* arrayName);

  vtkSMArrayRangeDomain(const vtkSMArrayRangeDomain&); // Not implemented
  void operator=(const vtkSMArrayRangeDomain&); // Not implemented
};

#endif
