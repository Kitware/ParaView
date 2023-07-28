// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSessionTypeDecorator.h"

#include "pqPropertyWidget.h"
#include "vtkLogger.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionClient.h"

//-----------------------------------------------------------------------------
pqSessionTypeDecorator::pqSessionTypeDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
  , IsVisible(true)
  , IsEnabled(true)
{
  auto proxy = parentObject->proxy();
  Q_ASSERT(proxy != nullptr);

  auto session = proxy->GetSession();
  Q_ASSERT(session != nullptr);

  bool condition_met = false;
  const char* requires = config->GetAttributeOrEmpty("requires");
  if (strcmp(requires, "remote") == 0)
  {
    condition_met = vtkSMSessionClient::SafeDownCast(session) != nullptr;
  }
  else if (strcmp(requires, "parallel") == 0)
  {
    condition_met =
      session->GetNumberOfProcesses(vtkPVSession::DATA_SERVER | vtkPVSession::RENDER_SERVER) > 1;
  }
  else if (strcmp(requires, "parallel_data_server") == 0)
  {
    condition_met = session->GetNumberOfProcesses(vtkPVSession::DATA_SERVER) > 1;
  }
  else if (strcmp(requires, "parallel_render_server") == 0)
  {
    condition_met = session->GetNumberOfProcesses(vtkPVSession::RENDER_SERVER) > 1;
  }
  else
  {
    vtkLogF(ERROR, "Invalid 'requires' attribute specified: '%s'", requires);
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
    vtkLogF(ERROR, "Invalid 'mode' attribute specified: '%s'", mode);
  }
}

//-----------------------------------------------------------------------------
pqSessionTypeDecorator::~pqSessionTypeDecorator() = default;

//-----------------------------------------------------------------------------
bool pqSessionTypeDecorator::canShowWidget(bool) const
{
  return this->IsVisible;
}

//-----------------------------------------------------------------------------
bool pqSessionTypeDecorator::enableWidget() const
{
  return this->IsEnabled;
}
