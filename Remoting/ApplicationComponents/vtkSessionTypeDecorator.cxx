// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSessionTypeDecorator.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionClient.h"

#include <cassert>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSessionTypeDecorator);

//-----------------------------------------------------------------------------
vtkSessionTypeDecorator::vtkSessionTypeDecorator() = default;

//-----------------------------------------------------------------------------
vtkSessionTypeDecorator::~vtkSessionTypeDecorator() = default;

//-----------------------------------------------------------------------------
void vtkSessionTypeDecorator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkSessionTypeDecorator::Initialize(vtkPVXMLElement* config, vtkSMProxy* proxy)
{
  this->Superclass::Initialize(config, proxy);

  auto session = proxy->GetSession();
  assert(session != nullptr);

  bool condition_met = false;
  const char* requiresAttr = config->GetAttributeOrEmpty("requires");
  if (strcmp(requiresAttr, "remote") == 0)
  {
    condition_met = vtkSMSessionClient::SafeDownCast(session) != nullptr;
  }
  else if (strcmp(requiresAttr, "parallel") == 0)
  {
    condition_met =
      session->GetNumberOfProcesses(vtkPVSession::DATA_SERVER | vtkPVSession::RENDER_SERVER) > 1;
  }
  else if (strcmp(requiresAttr, "parallel_data_server") == 0)
  {
    condition_met = session->GetNumberOfProcesses(vtkPVSession::DATA_SERVER) > 1;
  }
  else if (strcmp(requiresAttr, "parallel_render_server") == 0)
  {
    condition_met = session->GetNumberOfProcesses(vtkPVSession::RENDER_SERVER) > 1;
  }
  else
  {
    vtkErrorMacro(<< "Invalid 'requires' attribute specified: '" << requiresAttr << "'");
  }

  const char* mode = config->GetAttributeOrEmpty("mode");
  if (strcmp(mode, "visibility") == 0)
  {
    this->IsVisible = condition_met;
  }
  else if (strcmp(mode, "enabled_state") == 0)
  {
    this->IsEnabled = condition_met;
  }
  else
  {
    vtkErrorMacro(<< "Invalid 'mode' attribute specified: " << mode);
  }
}

//-----------------------------------------------------------------------------
bool vtkSessionTypeDecorator::Enable() const
{
  return this->IsEnabled;
}

//-----------------------------------------------------------------------------
bool vtkSessionTypeDecorator::CanShow(bool show_advanced) const
{
  (void)(show_advanced);
  return this->IsVisible;
}
