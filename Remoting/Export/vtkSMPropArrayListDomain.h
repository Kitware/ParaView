// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMPropArrayListDomain
 * @brief   list of arrays obtained from input prop
 *
 * This domain can only be used inside of a vtkSMRenderViewExporterProxy
 * It allows selecting arrays for all props in the view. The exporter needs to be able to handle
 * array selection for props in the form "propName:arrayName".
 */

#ifndef vtkSMPropArrayListDomain_h
#define vtkSMPropArrayListDomain_h

#include "vtkRemotingExportModule.h" //needed for exports
#include "vtkSMArrayListDomain.h"

class vtkPVDataSetAttributesInformation;
class vtkSMInputArrayDomain;
class vtkSMProxyProperty;
class vtkSMSourceProxy;
class vtkPVArrayInformation;

class vtkSMPropArrayListDomainInternals;

class VTKREMOTINGEXPORT_EXPORT vtkSMPropArrayListDomain : public vtkSMArrayListDomain
{
public:
  static vtkSMPropArrayListDomain* New();
  vtkTypeMacro(vtkSMPropArrayListDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Updates the string list based on the available arrays for each prop.
   */
  void Update(vtkSMProperty* prop) override;

protected:
  vtkSMPropArrayListDomain() = default;
  ~vtkSMPropArrayListDomain() override = default;

  /**
   * Read the mandatory array_type attribute for the domain, which must be either "point" or "cell"
   */
  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) override;

private:
  vtkSMPropArrayListDomain(const vtkSMPropArrayListDomain&) = delete;
  void operator=(const vtkSMPropArrayListDomain&) = delete;

  int ArrayType = 0;
};

#endif
