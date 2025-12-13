// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMDynamicPropertiesDomain
 * @brief   A domain (json description) for dynamic properties
 *
 * See:
 * - vtkPVRenderView::GetANARIRendererParameters for the json
 * description of properties associated with an ANARI renderer
 * - pqDynamicPropertiesWidget for the panel_widget for these
 * properties and
 * - ANARIRenderParameter XML property definiton for the RenderViewProxy
 * in view_removingviews.xml
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
