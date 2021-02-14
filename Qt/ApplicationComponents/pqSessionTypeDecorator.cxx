/*=========================================================================

   Program: ParaView
   Module:  pqSessionTypeDecorator.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
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
