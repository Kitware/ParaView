/*=========================================================================

   Program: ParaView
   Module:    pqDisplayPolicy.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"

#include <QtDebug>
#include <QString>

#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqPlotView.h"
#include "pqRenderView.h"
#include "pqServer.h"

//-----------------------------------------------------------------------------
pqDisplayPolicy::pqDisplayPolicy(QObject* _parent) :QObject(_parent)
{
}

//-----------------------------------------------------------------------------
pqDisplayPolicy::~pqDisplayPolicy()
{
}

//-----------------------------------------------------------------------------
QString pqDisplayPolicy::getPreferredViewType(pqOutputPort* opPort,
  bool update_pipeline) const
{
  pqPipelineSource* source = opPort->getSource();
  
  vtkPVXMLElement* hints = source->getHints();
  vtkPVXMLElement* viewElement = hints? 
    hints->FindNestedElementByName("View") : 0;
  QString view_type = viewElement ? 
    QString(viewElement->GetAttribute("type")) : QString::null;

  if (!view_type.isNull())
    {
    return view_type;
    }

  // HACK: for now, when update_pipeline is false, we don't do any gather
  // information as that can result in progress events which may case Qt paint
  // issues.
  vtkSMSourceProxy* spProxy = vtkSMSourceProxy::SafeDownCast(
    source->getProxy());
  if (!spProxy || (!update_pipeline && !spProxy->GetOutputPortsCreated()))
    {
    // If parts aren't created, don't update the information at all.
    // Typically means that the filter hasn't been "Applied" even once and
    // updating information on it may raise errors.
    return view_type;
    }

  vtkPVDataInformation* datainfo = update_pipeline?
    opPort->getDataInformation(true) : opPort->getCachedDataInformation();
  QString className = datainfo?  datainfo->GetDataClassName() : QString();
  if (className != "vtkRectilinearGrid")
    {
    return view_type;
    }

  // The proxy gives us no hint. In that case we try to determine the
  // preferred view by looking at the output from the source.
  if (datainfo)
    {
    int extent[6];
    datainfo->GetExtent(extent);
    int non_zero_dims = 0;
    for (int cc=0; cc < 3; cc++)
      {
      non_zero_dims += (extent[2*cc+1]-extent[2*cc]>0)? 1: 0;
      }

    vtkPVDataSetAttributesInformation* cellDataInfo =
      datainfo->GetCellDataInformation();
    vtkPVDataSetAttributesInformation* pointDataInfo =
      datainfo->GetPointDataInformation();
    if (non_zero_dims == 1 && cellDataInfo->GetNumberOfArrays() > 0)
      {
      // Has cell data, mostlikely this is a histogram.
      view_type = pqPlotView::barChartType();
      }
    else if (non_zero_dims == 1 && 
      (pointDataInfo->GetNumberOfArrays() > 0 ||
       cellDataInfo->GetNumberOfArrays() > 0 ) && 
      datainfo->GetNumberOfPoints() > 1)
      {
      // No cell data, but some point data -- may be a XY line plot.
      view_type = pqPlotView::XYPlotType();
      }
    }

  return view_type;
}

//-----------------------------------------------------------------------------
pqView* pqDisplayPolicy::getPreferredView(
  pqOutputPort* opPort, pqView* currentView) const
{
  pqObjectBuilder* builder = 
    pqApplicationCore::instance()->getObjectBuilder();
  QString view_type = this->getPreferredViewType(opPort, true); 

  if (!view_type.isNull())
    {
    QString proxy_name = view_type;
    if (currentView && currentView->getProxy()->GetXMLName() == proxy_name)
      {
      // nothing to do, active view is preferred view.
      }
    else
      {
      // Create the preferred view only if the current one is not of the same type
      // as the preferred view.
      currentView = builder->createView(view_type, opPort->getServer());
      }
    }

  if (!currentView)
    {
    // The user has selected a frame that is empty or the source does not
    // recommend any view type. Hence we create a render view.
    currentView = builder->createView(pqRenderView::renderViewType(),
      opPort->getServer());
    }

  // No hints. We don't know what type of view is suitable
  // for this proxy. Just check if it can be shown in current view.
  return  currentView;
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqDisplayPolicy::createPreferredRepresentation(
  pqOutputPort* opPort, pqView* view, bool dont_create_view) const
{

  if (!view && dont_create_view)
    {
    return NULL;
    }

  if (dont_create_view && (view && !view->canDisplay(opPort)))
    {
    return NULL;
    }

  if (!dont_create_view) 
    {
    view = this->getPreferredView(opPort, view); 
    if (!view)
      {
      // Could not create a view suitable for this source.
      // Creation of the display can no longer proceed.
      return NULL;
      }
    }

  // Simply create a display for the view set up the connections and
  // return.
  pqDataRepresentation* display = pqApplicationCore::instance()->
    getObjectBuilder()->createDataRepresentation(opPort, view);

  // If this is the only source displayed in the view, reset the camera to make sure its visible
  if(view->getNumberOfVisibleRepresentations()==1)
    {
    pqRenderView* ren = qobject_cast<pqRenderView*>(view);
    if (ren)
      {
      ren->resetCamera();
      }
    }

  return display;
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqDisplayPolicy::setRepresentationVisibility(
  pqOutputPort* opPort, pqView* view, bool visible) 
{
  if (!opPort)
    {
    // Cannot really repr a NULL source.
    return 0;
    }

  pqDataRepresentation* repr = opPort->getRepresentation(view);

  if (!repr && !visible)
    {
    // isn't visible already, nothing to change.
    return 0;
    }
  else if(!repr)
    {
    // FIXME:UDA -- can't we simply use createPreferredRepresentation?
    // No repr exists for this view.
    // First check if the view exists. If not, we will create a "suitable" view.
    if (!view)
      {
      view = this->getPreferredView(opPort, view);
      }
    if (view)
      {
      repr = pqApplicationCore::instance()->getObjectBuilder()->
        createDataRepresentation(opPort, view);
      }
    }

  repr->setVisible(visible);

  // If this is the only source displayed in the view, reset the camera to make 
  // sure its visible. Only do so if a source is being turned ON. Otherwise when 
  // the next to last source is turned off, the camera would be reset to fit the 
  // last remaining one which would be unexpected to the user 
  // (hence the conditional on "visible")
  if(view->getNumberOfVisibleRepresentations()==1 && visible)
    {
    pqRenderView* ren = qobject_cast<pqRenderView*>(view);
    if (ren)
      {
      ren->resetCamera();
      }
    }

  return repr;
}


