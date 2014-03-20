/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIntRangeDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMIntRangeDomain - type specific extension to
// vtkSMRangeDomainTemplate for ints.
// .SECTION Description
// vtkSMIntRangeDomain is a type specific extension to
// vtkSMRangeDomainTemplate for ints.
#ifndef __vtkSMIntRangeDomain_h
#define __vtkSMIntRangeDomain_h

// Tell the template header how to give our superclass a DLL interface.
#if !defined(__vtkSMIntRangeDomain_cxx)
# define VTK_DATA_ARRAY_TEMPLATE_TYPE int
#endif

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMDomain.h"
#include "vtkSMRangeDomainTemplate.h" // Read superclass

#ifndef __WRAP__
#define vtkSMDomain vtkSMRangeDomainTemplate<int>
#endif
class VTKPVSERVERMANAGERCORE_EXPORT vtkSMIntRangeDomain : public vtkSMDomain
#ifndef __WRAP__
#undef vtkSMDomain
#endif
{
public:
  static vtkSMIntRangeDomain* New();
  vtkTypeMacro(vtkSMIntRangeDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return a min. value if it exists. If the min. exists
  // exists is set to 1. Otherwise, it is set to 0.
  // An unspecified min. is equivalent to -inf
  int GetMinimum(unsigned int idx, int& exists)
    { return this->RealSuperclass::GetMinimum(idx, exists); }

  // Description:
  // Return a max. value if it exists. If the max. exists
  // exists is set to 1. Otherwise, it is set to 0.
  // An unspecified max. is equivalent to +inf
  int GetMaximum(unsigned int idx, int& exists)
    { return this->RealSuperclass::GetMaximum(idx, exists); }

  // Description:
  // Returns if minimum/maximum bound is set for the domain.
  int GetMinimumExists(unsigned int idx)
    { return this->RealSuperclass::GetMinimumExists(idx)? 1: 0; }
  int GetMaximumExists(unsigned int idx)
    { return this->RealSuperclass::GetMaximumExists(idx)? 1: 0; }

  // Description:
  // Returns the minimum/maximum value, is exists, otherwise
  // 0 is returned. Use GetMaximumExists() GetMaximumExists() to make sure that
  // the bound is set.
  int GetMinimum(unsigned int idx)
    { return this->RealSuperclass::GetMinimum(idx); }
  int GetMaximum(unsigned int idx)
    { return this->RealSuperclass::GetMaximum(idx); }

protected:
  vtkSMIntRangeDomain();
  ~vtkSMIntRangeDomain();

private:
  vtkSMIntRangeDomain(const vtkSMIntRangeDomain&); // Not implemented
  void operator=(const vtkSMIntRangeDomain&); // Not implemented

  typedef vtkSMRangeDomainTemplate<int> RealSuperclass;
};

#if !defined(__vtkSMIntRangeDomain_cxx)
# undef VTK_DATA_ARRAY_TEMPLATE_TYPE
#endif

#endif
