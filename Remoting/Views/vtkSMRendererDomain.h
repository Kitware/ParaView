// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMRendererDomain
 * @brief   Manages the list of available ray traced renderers
 * This domain builds the list of ray traced renderer backends on the
 * View section of the Qt GUI.
 */

#ifndef vtkSMRendererDomain_h
#define vtkSMRendererDomain_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMStringListDomain.h"

class VTKREMOTINGVIEWS_EXPORT vtkSMRendererDomain : public vtkSMStringListDomain
{
public:
  static vtkSMRendererDomain* New();
  vtkTypeMacro(vtkSMRendererDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Update(vtkSMProperty*) override;

protected:
  vtkSMRendererDomain() = default;
  ~vtkSMRendererDomain() override = default;

  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) override;

private:
  vtkSMRendererDomain(const vtkSMRendererDomain&) = delete;
  void operator=(const vtkSMRendererDomain&) = delete;
};

#endif
