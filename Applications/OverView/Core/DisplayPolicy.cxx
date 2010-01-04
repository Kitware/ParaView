/*=========================================================================

   Program: ParaView
   Module:    DisplayPolicy.cxx

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

#include "DisplayPolicy.h"

#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkStructuredData.h"

#include <QtDebug>
#include <QString>

#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqTwoDRenderView.h"

//-----------------------------------------------------------------------------
DisplayPolicy::DisplayPolicy(QObject* _parent) : pqDisplayPolicy(_parent)
{
}

//-----------------------------------------------------------------------------
DisplayPolicy::~DisplayPolicy()
{
}


//-----------------------------------------------------------------------------
pqDataRepresentation* DisplayPolicy::createPreferredRepresentation(
  pqOutputPort* opPort, pqView* view, bool dont_create_view) const
{
  vtkPVDataInformation* selectedInformation = opPort->getDataInformation(false);
  vtkSMSourceProxy* selectionProxy = 
    vtkSMSourceProxy::SafeDownCast(opPort->getSource()->getProxy());
  if(view && selectedInformation->GetDataSetType() == VTK_SELECTION)
    {
    // Find the view's first visible representation
    QList<pqRepresentation*> reps = view->getRepresentations();
    pqDataRepresentation* pqRepr = NULL;
    for(int i=0; i<reps.size(); ++i)
      {
      if(reps[i]->isVisible())
        {
        pqRepr = qobject_cast<pqDataRepresentation*>(reps[i]);
        break;
        }
      } 
    // No visible rep
    if(!pqRepr)
      return 0;

    vtkSMSourceProxy* srcProxy = vtkSMSourceProxy::SafeDownCast(
      pqRepr->getOutputPortFromInput()->getSource()->getProxy());
    srcProxy->SetSelectionInput(opPort->getPortNumber(), selectionProxy, 0);

    return pqRepr;
    }

  return pqDisplayPolicy::createPreferredRepresentation(opPort, view, dont_create_view);
}

//-----------------------------------------------------------------------------
pqDataRepresentation* DisplayPolicy::setRepresentationVisibility(
  pqOutputPort* opPort, pqView* view, bool visible) const
{
  vtkPVDataInformation* selectedInformation = opPort->getDataInformation(false);
  vtkSMSourceProxy* selectionProxy = 
    vtkSMSourceProxy::SafeDownCast(opPort->getSource()->getProxy());
  if(view && selectedInformation->GetDataSetType() == VTK_SELECTION)
    {
    if (!opPort)
      {
      // Cannot really repr a NULL source.
      return 0;
      }
  
    // Find the view's first visible representation
    QList<pqRepresentation*> reps = view->getRepresentations();
    pqDataRepresentation* pqRepr = NULL;
    for(int i=0; i<reps.size(); ++i)
      {
      if(reps[i]->isVisible())
        {
        pqRepr = qobject_cast<pqDataRepresentation*>(reps[i]);
        break;
        }
      } 

    // No visible rep
    if(!pqRepr)
      return 0;

    vtkSMSourceProxy* srcProxy = vtkSMSourceProxy::SafeDownCast(
      pqRepr->getOutputPortFromInput()->getSource()->getProxy());

    if(visible)
      {
      srcProxy->SetSelectionInput(opPort->getPortNumber(), selectionProxy, 0);
      }
    else
      {
      srcProxy->CleanSelectionInputs(opPort->getPortNumber());
      }

    return pqRepr;
    }

  return pqDisplayPolicy::setRepresentationVisibility(opPort,view,visible);
}


//-----------------------------------------------------------------------------
pqDisplayPolicy::VisibilityState DisplayPolicy::getVisibility(
  pqView* view, pqOutputPort* port) const
{
  if(!view && this->getPreferredViewType(port, false) == QString::null)
    {
      // No repr exists, not can one be created.
      return NotApplicable;
    }

  return pqDisplayPolicy::getVisibility(view, port);
}

