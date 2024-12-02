// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMPropDomain
 * @brief   list of props currently shown in the active view
 *
 * Can only be used inside of a vtkSMRenderViewExporterProxy
 *
 */

#ifndef vtkSMPropDomain_h
#define vtkSMPropDomain_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMStringListDomain.h"

class vtkPVDataSetAttributesInformation;
class vtkSMInputArrayDomain;
class vtkSMProxyProperty;
class vtkSMSourceProxy;
class vtkPVArrayInformation;

class vtkSMPropDomainInternals;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMPropDomain : public vtkSMStringListDomain
{
public:
  static vtkSMPropDomain* New();
  vtkTypeMacro(vtkSMPropDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Updates the string list based on source names of props currently displayed in the view.
   */
  void Update(vtkSMProperty* prop) override;

protected:
  vtkSMPropDomain();
  ~vtkSMPropDomain() override;

private:
  vtkSMPropDomain(const vtkSMPropDomain&) = delete;
  void operator=(const vtkSMPropDomain&) = delete;

  friend class vtkSMPropDomainInternals;
  vtkSMPropDomainInternals* ALDInternals;
};

#endif
