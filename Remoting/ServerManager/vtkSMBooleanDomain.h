// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMBooleanDomain
 * @brief   a domain with two values: true or false
 *
 * vtkSMBooleanDomain does not really restrict the values of the property
 * that contains it. All integer values are valid. Rather, it is used to
 * specified that the property represents a true/false state. This domains
 * works with only vtkSMIntVectorProperty.
 * @sa
 * vtkSMDomain vtkSMIntVectorProperty
 */

#ifndef vtkSMBooleanDomain_h
#define vtkSMBooleanDomain_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMDomain.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMBooleanDomain : public vtkSMDomain
{
public:
  static vtkSMBooleanDomain* New();
  vtkTypeMacro(vtkSMBooleanDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns true if the property is a vtkSMIntVectorProperty.
   * Return 0 otherwise.
   */
  int IsInDomain(vtkSMProperty* property) override;

  /**
   * Set the value of an element of a property from the animation editor.
   */
  void SetAnimationValue(vtkSMProperty* property, int idx, double value) override;

protected:
  vtkSMBooleanDomain();
  ~vtkSMBooleanDomain() override;

private:
  vtkSMBooleanDomain(const vtkSMBooleanDomain&) = delete;
  void operator=(const vtkSMBooleanDomain&) = delete;
};

#endif
