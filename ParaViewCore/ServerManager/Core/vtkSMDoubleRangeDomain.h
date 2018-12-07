/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDoubleRangeDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMDoubleRangeDomain
 * @brief   type specific extension to
 * vtkSMRangeDomainTemplate for doubles.
 *
 * vtkSMDoubleRangeDomain is a type-specific specialization for
 * vtkSMRangeDomainTemplate.
*/

#ifndef vtkSMDoubleRangeDomain_h
#define vtkSMDoubleRangeDomain_h

// Tell the template header how to give our superclass a DLL interface.
#if !defined(vtkSMDoubleRangeDomain_cxx)
#define VTK_DATA_ARRAY_TEMPLATE_TYPE double
#endif

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMDomain.h"
#include "vtkSMRangeDomainTemplate.h" // Read superclass

// Fake the superclass for the wrappers.
#ifndef __WRAP__
#define vtkSMDomain vtkSMRangeDomainTemplate<double>
#endif
class VTKPVSERVERMANAGERCORE_EXPORT vtkSMDoubleRangeDomain : public vtkSMDomain
#ifndef __WRAP__
#undef vtkSMDomain
#endif
{
public:
  static vtkSMDoubleRangeDomain* New();
  vtkTypeMacro(vtkSMDoubleRangeDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return a min. value if it exists. If the min. exists
   * exists is set to 1. Otherwise, it is set to 0.
   * An unspecified min. is equivalent to -inf
   */
  double GetMinimum(unsigned int idx, int& exists)
  {
    return this->RealSuperclass::GetMinimum(idx, exists);
  }

  /**
   * Return a max. value if it exists. If the max. exists
   * exists is set to 1. Otherwise, it is set to 0.
   * An unspecified max. is equivalent to +inf
   */
  double GetMaximum(unsigned int idx, int& exists)
  {
    return this->RealSuperclass::GetMaximum(idx, exists);
  }

  /**
   * Returns if minimum/maximum bound is set for the domain.
   */
  int GetMinimumExists(unsigned int idx)
  {
    return this->RealSuperclass::GetMinimumExists(idx) ? 1 : 0;
  }
  int GetMaximumExists(unsigned int idx)
  {
    return this->RealSuperclass::GetMaximumExists(idx) ? 1 : 0;
  }

  /**
   * Returns the minimum/maximum value, is exists, otherwise
   * 0 is returned. Use GetMaximumExists() GetMaximumExists() to make sure that
   * the bound is set.
   */
  double GetMinimum(unsigned int idx) { return this->RealSuperclass::GetMinimum(idx); }
  double GetMaximum(unsigned int idx) { return this->RealSuperclass::GetMaximum(idx); }

  /**
   * Returns the resolution.
   */
  int GetResolution() { return this->RealSuperclass::GetResolution(); }

protected:
  vtkSMDoubleRangeDomain();
  ~vtkSMDoubleRangeDomain() override;

private:
  vtkSMDoubleRangeDomain(const vtkSMDoubleRangeDomain&) = delete;
  void operator=(const vtkSMDoubleRangeDomain&) = delete;

  typedef vtkSMRangeDomainTemplate<double> RealSuperclass;
};

#if !defined(vtkSMDoubleRangeDomain_cxx)
#undef VTK_DATA_ARRAY_TEMPLATE_TYPE
#endif

#endif
