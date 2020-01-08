/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRangedTransferFunctionDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMRangedTransferFunctionDomain
 * @brief   represents a ranged vtkSMTransferFunctionProxy
 *
 * This domain represent a single vtkSMTransferFunctionProxy with
 * a range defined by a vtkSMArrayRangeDomain used internally.
 *
 * Example usage :
 * @code{xml}
 *       <RangedTransferFunctionDomain name="domain">
 *         <RequiredProperties>
 *           <Property function="Input" name="Input" />
 *           <Property function="ArraySelection" name="OpacityArray" />
 *           <Property function="ComponentSelection" name="OpacityArrayComponent" />
 *         </RequiredProperties>
 *       </RangedTransferFunctionDomain>
 * @endcode
 */

#ifndef vtkSMRangedTransferFunctionDomain_h
#define vtkSMRangedTransferFunctionDomain_h

#include "vtkClientServerID.h"      // needed for saving animation in batch script
#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMProxyListDomain.h"

class vtkSMRangedTransferFunctionDomainInternals;

class VTKREMOTINGVIEWS_EXPORT vtkSMRangedTransferFunctionDomain : public vtkSMProxyListDomain
{
public:
  static vtkSMRangedTransferFunctionDomain* New();
  vtkTypeMacro(vtkSMRangedTransferFunctionDomain, vtkSMProxyListDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return a range min. value if it exists. If the min. exists
   * exists is set to 1. Otherwise, it is set to 0.
   * An unspecified min. is equivalent to -inf
   */
  double GetRangeMinimum(unsigned int idx, int& exists);

  /**
   * Return a range max. value if it exists. If the max. exists
   * exists is set to 1. Otherwise, it is set to 0.
   * An unspecified max. is equivalent to +inf
   */
  double GetRangeMaximum(unsigned int idx, int& exists);

  //@{
  /**
   * Returns if range minimum/maximum bound is set for the domain.
   */
  bool GetRangeMinimumExists(unsigned int idx);
  bool GetRangeMaximumExists(unsigned int idx);
  //@}

  //@{
  /**
   * Returns the range minimum/maximum value, is exists, otherwise
   * 0 is returned. Use GetMaximumExists() GetMaximumExists() to make sure that
   * the bound is set.
   */
  double GetRangeMinimum(unsigned int idx)
  {
    int not_used;
    return this->GetRangeMinimum(idx, not_used);
  }
  double GetRangeMaximum(unsigned int idx)
  {
    int not_used;
    return this->GetRangeMaximum(idx, not_used);
  }
  //@}

  /**
   * If prop is a vtkSMProxyProperty containing a vtkSMTransferFunctionProxy
   * this set its range based on this domain min and max values.
   */
  int SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values) override;

protected:
  vtkSMRangedTransferFunctionDomain();
  ~vtkSMRangedTransferFunctionDomain() override;

  /**
   * Set the appropriate ivars from the xml element. Should
   * be overwritten by subclass if adding ivars.
   */
  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) override;

  /**
   * Callback to invoke DomainModified event
   */
  void InvokeDomainModifiedEvent();

private:
  vtkSMRangedTransferFunctionDomain(const vtkSMRangedTransferFunctionDomain&) = delete;
  void operator=(const vtkSMRangedTransferFunctionDomain&) = delete;

  vtkSMRangedTransferFunctionDomainInternals* Internals;
};

#endif
