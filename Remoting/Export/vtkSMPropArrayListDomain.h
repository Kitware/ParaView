// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMPropArrayListDomain
 * @brief   list of arrays obtained from input prop
 *
 * Can only be used inside of a vtkSMRenderViewExporterProxy
 *
 */

#ifndef vtkSMPropArrayListDomain_h
#define vtkSMPropArrayListDomain_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMArrayListDomain.h"
#include "vtkSMStringListDomain.h"

class vtkPVDataSetAttributesInformation;
class vtkSMInputArrayDomain;
class vtkSMProxyProperty;
class vtkSMSourceProxy;
class vtkPVArrayInformation;

class vtkSMPropArrayListDomainInternals;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMPropArrayListDomain : public vtkSMStringListDomain
{
public:
  static vtkSMPropArrayListDomain* New();
  vtkTypeMacro(vtkSMPropArrayListDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Updates the string list based on the available arrays. Requires
   * a property of class vtkSMProxyProperty which points to a
   * vtkSMSourceProxy and contains a vtkSMInputArrayDomain. Only
   * the first proxy and domain are used.
   */
  void Update(vtkSMProperty* prop) override;

  // int IsArrayPartial(unsigned int idx) override { return false; };

  // int GetFieldAssociation(unsigned int idx) override { return 0; };

  // int GetDomainAssociation(unsigned int idx) override { return 0;};

  int GetAttributeType() { return 0; }

  /**
   * A vtkSMProperty is often defined with a default value in the
   * XML itself. However, many times, the default value must be determined
   * at run time. To facilitate this, domains can override this method
   * to compute and set the default value for the property.
   * Note that unlike the compile-time default values, the
   * application must explicitly call this method to initialize the
   * property.
   * Returns 1 if the domain updated the property.
   */
  int SetDefaultValues(vtkSMProperty*, bool use_unchecked_values) override;

protected:
  vtkSMPropArrayListDomain();
  ~vtkSMPropArrayListDomain() override;

  /**
   * Set the appropriate ivars from the xml element. Should
   * be overwritten by subclass if adding ivars.
   */
  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) override;

private:
  vtkSMPropArrayListDomain(const vtkSMPropArrayListDomain&) = delete;
  void operator=(const vtkSMPropArrayListDomain&) = delete;

  friend class vtkSMPropArrayListDomainInternals;
  vtkSMPropArrayListDomainInternals* ALDInternals;
};

#endif
