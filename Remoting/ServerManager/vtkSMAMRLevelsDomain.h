// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMAMRLevelsDomain
 * @brief   int range domain based on the levels in AMR data.
 *
 * vtkSMAMRLevelsDomain is a subclass of vtkSMIntRangeDomain. It relies on one
 * required property: "Input". "Input" is vtkSMInputProperty which provides the
 * information about the data.
 *
 * Supported Required-Property functions:
 * \li \c Input : points to a property that provides the data producer.
 */

#ifndef vtkSMAMRLevelsDomain_h
#define vtkSMAMRLevelsDomain_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMIntRangeDomain.h"

class vtkSMIntVectorProperty;
class vtkSMProxyProperty;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMAMRLevelsDomain : public vtkSMIntRangeDomain
{
public:
  static vtkSMAMRLevelsDomain* New();
  vtkTypeMacro(vtkSMAMRLevelsDomain, vtkSMIntRangeDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Update the domain using the "unchecked" values (if available) for all
   * required properties.
   */
  void Update(vtkSMProperty*) override;

protected:
  vtkSMAMRLevelsDomain();
  ~vtkSMAMRLevelsDomain() override;

  vtkPVDataInformation* GetInputInformation();

private:
  vtkSMAMRLevelsDomain(const vtkSMAMRLevelsDomain&) = delete;
  void operator=(const vtkSMAMRLevelsDomain&) = delete;
};

#endif
