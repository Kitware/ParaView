// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMDomain.h"
#include "vtkSMRangeDomainTemplate.h" // Read superclass

// Fake the superclass for the wrappers.
#ifndef __WRAP__
#define vtkSMDomain vtkSMRangeDomainTemplate<double>
#endif
class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMDoubleRangeDomain : public vtkSMDomain
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
  virtual double GetMinimum(unsigned int idx, int& exists)
  {
    return this->RealSuperclass::GetMinimum(idx, exists);
  }

  /**
   * Return a max. value if it exists. If the max. exists
   * exists is set to 1. Otherwise, it is set to 0.
   * An unspecified max. is equivalent to +inf
   */
  virtual double GetMaximum(unsigned int idx, int& exists)
  {
    return this->RealSuperclass::GetMaximum(idx, exists);
  }

  /**
   * Returns if minimum/maximum bound is set for the domain.
   */
  virtual int GetMinimumExists(unsigned int idx)
  {
    return this->RealSuperclass::GetMinimumExists(idx) ? 1 : 0;
  }
  virtual int GetMaximumExists(unsigned int idx)
  {
    return this->RealSuperclass::GetMaximumExists(idx) ? 1 : 0;
  }

  /**
   * Returns the minimum/maximum value, is exists, otherwise
   * 0 is returned. Use GetMaximumExists() GetMaximumExists() to make sure that
   * the bound is set.
   */
  virtual double GetMinimum(unsigned int idx) { return this->RealSuperclass::GetMinimum(idx); }
  virtual double GetMaximum(unsigned int idx) { return this->RealSuperclass::GetMaximum(idx); }

  /**
   * Returns the resolution.
   */
  virtual int GetResolution() { return this->RealSuperclass::GetResolution(); }

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
