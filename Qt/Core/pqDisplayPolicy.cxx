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
#include "vtkSMAbstractDisplayProxy.h"
#include "vtkSMAbstractViewModuleProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"

#include <QtDebug>
#include <QString>

#include "pqApplicationCore.h"
#include "pqConsumerDisplay.h"
#include "pqObjectBuilder.h"
#include "pqPipelineSource.h"
#include "pqPlotViewModule.h"
#include "pqRenderViewModule.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"

//-----------------------------------------------------------------------------
pqDisplayPolicy::pqDisplayPolicy(QObject* _parent) :QObject(_parent)
{
}

//-----------------------------------------------------------------------------
pqDisplayPolicy::~pqDisplayPolicy()
{
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqDisplayPolicy::newDisplayProxy(
  pqPipelineSource* source, pqGenericViewModule* view) const
{
  if(view && !view->canDisplaySource(source))
    {
    return NULL;
    }
  QString srcProxyName = source->getProxy()->GetXMLName();
  if (srcProxyName == "TextSource" || srcProxyName == "TimeToTextConvertor")
    {
    vtkSMProxy* p = vtkSMObject::GetProxyManager()->NewProxy(
      "displays", "TextDisplay");
    p->SetConnectionID(view->getServer()->GetConnectionID());
    return p;
    }

  return view->getViewModuleProxy()->CreateDisplayProxy(); 
}

//-----------------------------------------------------------------------------
pqGenericViewModule* pqDisplayPolicy::getPreferredView(pqPipelineSource* source,
  pqGenericViewModule* currentView) const
{
  pqObjectBuilder* builder = 
    pqApplicationCore::instance()->getObjectBuilder();
  vtkPVXMLElement* hints = source->getHints();
  vtkPVXMLElement* viewElement = hints? 
    hints->FindNestedElementByName("View") : 0;
  QString view_type = viewElement ? QString(viewElement->GetAttribute("type")) : QString::null;

  if (view_type.isNull())
    {
    // The proxy gives us no hint. In that case we try to determine the
    // preferred view by looking at the output from the source.
    vtkPVDataInformation* datainfo = source->getDataInformation();
    if (datainfo && (
        datainfo->GetDataClassName() == QString("vtkRectilinearGrid")  ||
        source->getProxy()->GetXMLName() == QString("Probe")))
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
        view_type = pqPlotViewModule::barChartType();
        }
      else if (
        (source->getProxy()->GetXMLName() == QString("Probe") || 
         non_zero_dims == 1) && 
        (pointDataInfo->GetNumberOfArrays() > 0 ||
         cellDataInfo->GetNumberOfArrays() > 0 ) && 
        datainfo->GetNumberOfPoints() > 1)
        {
        // No cell data, but some point data -- may be a XY line plot.
        view_type = pqPlotViewModule::XYPlotType();
        }
      }
    }
  if (!view_type.isNull())
    {
    QString proxy_name = view_type;
    proxy_name += "ViewModule";
    if (currentView && currentView->getProxy()->GetXMLName() == proxy_name)
      {
      // nothing to do, active view is preferred view.
      }
    else
      {
      // Create the preferred view only if the current one is not of the same type
      // as the preferred view.
      currentView = builder->createView(view_type, source->getServer());
      }
    }

  if (!currentView)
    {
    // The user has selected a frame that is empty or the source does not
    // recommend any view type. Hence we create a render view.
    currentView = builder->createView(pqRenderViewModule::renderViewType(),
      source->getServer());
    }

  // No hints. We don't know what type of view is suitable
  // for this proxy. Just check if it can be shown in current view.
  return  currentView;
}

//-----------------------------------------------------------------------------
pqConsumerDisplay* pqDisplayPolicy::createPreferredDisplay(
  pqPipelineSource* source, pqGenericViewModule* view,
  bool dont_create_view) const
{
  if (source)
    {
    vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(source->getProxy());
    if (sp)
      {
      // ensure parts are created.
      sp->CreateParts();
      }
    }
  if (!view && dont_create_view)
    {
    return NULL;
    }

  if (dont_create_view && (view && !view->canDisplaySource(source)))
    {
    return NULL;
    }

  if (!dont_create_view) 
    {
    view = this->getPreferredView(source, view); 
    if (!view)
      {
      // Could not create a view suitable for this source.
      // Creation of the display can no longer proceed.
      return NULL;
      }
    }

  // Simply create a display for the view set up the connections and
  // return.
  pqConsumerDisplay* display = 
    pqApplicationCore::instance()->getObjectBuilder()->
    createDataDisplay(source, view);

  // If this is the only source displayed in the view, reset the camera to make sure its visible
  if(view->getVisibleDisplayCount()==1)
    {
    pqRenderViewModule* ren = qobject_cast<pqRenderViewModule*>(view);
    if (ren)
      {
      ren->resetCamera();
      ren->render();
      }
    }

  return display;
}

//-----------------------------------------------------------------------------
pqConsumerDisplay* pqDisplayPolicy::setDisplayVisibility(
  pqPipelineSource* source, pqGenericViewModule* view, bool visible) 
{
  if (!source)
    {
    // Cannot really display a NULL source.
    return 0;
    }

  pqConsumerDisplay* display = source->getDisplay(view);

  if (!display && !visible)
    {
    // isn't visible already, nothing to change.
    return 0;
    }
  else if(!display)
    {
    // No display exists for this view.
    // First check if the view exists. If not, we will create a "suitable" view.
    if (!view)
      {
      view = this->getPreferredView(source, view);
      }
    if (view)
      {
      display = pqApplicationCore::instance()->getObjectBuilder()->
        createDataDisplay(source, view);
      }
    }

  display->setVisible(visible);

  // If this is the only source displayed in the view, reset the camera to make sure its visible
  // Only do so if a source is being turned ON. Otherwise when the next to last source is turned off, 
  // the camera would be reset to fit the last remaining one which would be unexpected to the user (hence the conditional on "visible")
  if(view->getVisibleDisplayCount()==1 && visible)
    {
    pqRenderViewModule* ren = qobject_cast<pqRenderViewModule*>(view);
    if (ren)
      {
      ren->resetCamera();
      ren->render();
      }
    }

  return display;
}


