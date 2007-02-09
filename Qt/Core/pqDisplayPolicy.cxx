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
#include "vtkPVXMLElement.h"
#include "vtkSMAbstractDisplayProxy.h"
#include "vtkSMAbstractViewModuleProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"

#include <QtDebug>
#include <QString>

#include "pqApplicationCore.h"
#include "pqConsumerDisplay.h"
#include "pqGenericViewModule.h"
#include "pqPipelineBuilder.h"
#include "pqPipelineSource.h"
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
bool pqDisplayPolicy::canDisplay(const pqPipelineSource* source,
  const pqGenericViewModule* view) const
{
  if (!source || !view)
    {
    return false;
    }

  if (source->getServer()->GetConnectionID() != 
    view->getServer()->GetConnectionID())
    {
    return false;
    }

  // Based on type of view, we check is the source's output 
  // is of the expected type.
  QString viewProxyName = view->getProxy()->GetXMLName();
  QString srcProxyName = source->getProxy()->GetXMLName();

  if (viewProxyName == "BarChartViewModule")
    {
    vtkPVDataInformation* dataInfo = source->getDataInformation();
    if (dataInfo)
      {
      int extent[6];
      dataInfo->GetExtent(extent);
      int non_zero_dims = 0;
      for (int cc=0; cc < 3; cc++)
        {
        non_zero_dims += (extent[2*cc+1]-extent[2*cc]>0)? 1: 0;
        }

      return (dataInfo->GetDataClassName() == QString("vtkRectilinearGrid")) &&
        (non_zero_dims == 1);
      }
    }
  if (viewProxyName == "XYPlotViewModule")
    {
    return (srcProxyName == "Probe2");
    }

  return true;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqDisplayPolicy::newDisplayProxy(
  pqPipelineSource* source, pqGenericViewModule* view) const
{
  if (!this->canDisplay(source, view))
    {
    return NULL;
    }
  QString srcProxyName = source->getProxy()->GetXMLName();
  if (srcProxyName == "TextSource")
    {
    vtkSMProxy* p = vtkSMObject::GetProxyManager()->NewProxy(
      "displays", "TextWidgetDisplay");
    p->SetConnectionID(view->getServer()->GetConnectionID());
    return p;
    }

  return view->getViewModuleProxy()->CreateDisplayProxy(); 
}

//-----------------------------------------------------------------------------
pqGenericViewModule* pqDisplayPolicy::getPreferredView(pqPipelineSource* source,
  pqGenericViewModule* currentView) const
{
  pqPipelineBuilder* builder = 
    pqApplicationCore::instance()->getPipelineBuilder();
  vtkPVXMLElement* hints = source->getHints();
  vtkPVXMLElement* viewElement = hints? 
    hints->FindNestedElementByName("View") : 0;
  const char* view_type = viewElement? viewElement->GetAttribute("type") : 0;
  if (view_type)
    {
    QString proxy_name = view_type;
    proxy_name += "ViewModule";
    if (currentView && currentView->getProxy()->GetXMLName() == proxy_name)
      {
      // nothing to do, active view is preferred view.
      }
    else
      {
      // Create the preferred view only if one doesn't exist already.
      pqGenericViewModule *preferredView = 0;
      if (currentView)
        {
        // If currentView is empty, then we must always create a new view of the preferred type.
        QList<pqGenericViewModule*> views = 
          pqApplicationCore::instance()->getServerManagerModel()->getViewModules(
            source->getServer());
        foreach (pqGenericViewModule* view, views)
          {
          if (proxy_name == view->getProxy()->GetXMLName())
            {
            preferredView = view;
            break;
            }
          }
        }
      if (!preferredView)
        {
        // Preferred view does not exist (or we insist on creating a new one) create a new view.
        if (strcmp(view_type, "XYPlot")==0 )
          {
          currentView = builder->createView(
            pqGenericViewModule::XY_PLOT, source->getServer());
          }
        else if (strcmp(view_type,"BarChart") ==0 )
          {
          currentView = builder->createView(
            pqGenericViewModule::BAR_CHART, source->getServer());
          }
        }
      }
    }

  if (!currentView)
    {
    // The user has selected a frame that is empty or the source does not
    // recommend any view type. Hence we create a render view.
    currentView = builder->createView(
      pqGenericViewModule::RENDER_VIEW, source->getServer());
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

  if (dont_create_view && !this->canDisplay(source, view))
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
  pqConsumerDisplay* display = pqApplicationCore::instance()->getPipelineBuilder()->
    createDisplay(source, view);
  return display;
}

//-----------------------------------------------------------------------------
pqConsumerDisplay* pqDisplayPolicy::setDisplayVisibility(
  pqPipelineSource* source, pqGenericViewModule* view, bool visible) const
{
  if (!source)
    {
    // Cannot really display a NULL source.
    return 0;
    }

  pqConsumerDisplay* display = source->getDisplay(view);
  if (display)
    {
    // set the visibility.
    display->setVisible(visible);
    return display;
    }

  // No display exists for this view.
  // First check if the view exists. If not, we will create a "suitable" view.
  if (!view)
    {
    view = this->getPreferredView(source, view);
    }
  if (view)
    {
    display = pqApplicationCore::instance()->getPipelineBuilder()->createDisplay(source, view);
    display->setVisible(visible);
    }
  return display;
}


