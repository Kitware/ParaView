/*=========================================================================

   Program: ParaView
   Module:    pqDisplayPolicy.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include "pqDisplayPolicy.h"

#include <QString>
#include <QtDebug>

#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqView.h"
#include "vtkNew.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"

//-----------------------------------------------------------------------------
pqDisplayPolicy::pqDisplayPolicy(QObject* _parent)
  : QObject(_parent)
{
}

//-----------------------------------------------------------------------------
pqDisplayPolicy::~pqDisplayPolicy()
{
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqDisplayPolicy::setRepresentationVisibility(
  pqOutputPort* opPort, pqView* view, bool visible) const
{
  if (!opPort)
  {
    // Cannot really repr a NULL source.
    return 0;
  }

  if (opPort->getServer() && opPort->getServer()->getResource().scheme() == "catalyst")
  {
    return 0;
  }

  vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(opPort->getSource()->getProxy());
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
  vtkSMProxy* reprProxy = controller->SetVisibility(
    source, opPort->getPortNumber(), (view ? view->getViewProxy() : NULL), visible);

  return pqApplicationCore::instance()->getServerManagerModel()->findItem<pqDataRepresentation*>(
    reprProxy);
}

//-----------------------------------------------------------------------------
QString pqDisplayPolicy::getPreferredViewType(pqOutputPort* port, bool update_pipeline) const
{
  (void)update_pipeline;
  if (port)
  {
    vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(port->getSource()->getProxy());
    vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
    return QString(controller->GetPreferredViewType(source, port->getPortNumber()));
  }
  return QString();
}

//-----------------------------------------------------------------------------
pqDisplayPolicy::VisibilityState pqDisplayPolicy::getVisibility(
  pqView* view, pqOutputPort* port) const
{
  if (view && port)
  {
    vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(port->getSource()->getProxy());

    vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
    if (controller->GetVisibility(source, port->getPortNumber(), view->getViewProxy()))
    {
      return Visible;
    }

    if (view->getViewProxy()->CanDisplayData(source, port->getPortNumber()))
    {
      // If repr exists, or a new repr can be created for the port (since port
      // is show-able in the view)
      return Hidden;
    }
    else
    {
      // No repr exists, not can one be created.
      return NotApplicable;
    }
  }

  //// If the port is on a CatalystSession or it hasn't been initialized yet,
  //// it has "no visiblily", so to speak.
  // if (port && port->getServer() &&
  //  port->getServer()->getResource().scheme() == "catalyst")
  //  {
  //  return NotApplicable;
  //  }
  // if (port && port->getSource() &&
  //  port->getSource()->modifiedState() == pqProxy::UNINITIALIZED)
  //  {
  //  return NotApplicable;
  //  }

  // Default behavior if no view is present
  return Hidden;
}
