// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMDynamicPropertiesDomain
 * @brief   Select names from an indexed string list.
 *
 *
 * See the vtkMPASReader proxy in readers.xml for how the properties should be
 * set up for this domain.
 */

#ifndef vtkSMDynamicPropertiesDomain_h
#define vtkSMDynamicPropertiesDomain_h

#include "vtkRemotingServerManagerModule.h" // For export macro
#include "vtkSMDomain.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMDynamicPropertiesDomain : public vtkSMDomain
{
public:
  vtkTypeMacro(vtkSMDynamicPropertiesDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkSMDynamicPropertiesDomain* New();

  int IsInDomain(vtkSMProperty* property) override;

  int SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values) override;

  vtkSMProperty* GetInfoProperty() { return this->GetRequiredProperty("Info"); }

protected:
  vtkSMDynamicPropertiesDomain();
  ~vtkSMDynamicPropertiesDomain() override;

private:
  vtkSMDynamicPropertiesDomain(const vtkSMDynamicPropertiesDomain&) = delete;
  void operator=(const vtkSMDynamicPropertiesDomain&) = delete;
};

#endif // vtkSMDynamicPropertiesDomain_h
