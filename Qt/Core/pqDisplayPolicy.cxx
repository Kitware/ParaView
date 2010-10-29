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
#include "pqSpreadSheetView.h"
#include "pqTwoDRenderView.h"
#include "pqXYBarChartView.h"
#include "pqXYChartView.h"

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
  QString view_type = QString::null;

  if (!opPort)
    {
    return view_type;
    }

  pqPipelineSource* source = opPort->getSource();
  if (update_pipeline)
    {
    source->updatePipeline();
    }

  // Some sources have hints that say that the output is to be treated as raw
  // text. We flag such sources.
  bool is_text = false;

  vtkPVXMLElement* hints = source->getHints();
  if (hints)
    {
    for (unsigned int cc=0; cc < hints->GetNumberOfNestedElements(); cc++)
      {
      vtkPVXMLElement* child = hints->GetNestedElement(cc);
      if (child && child->GetName())
        {
        if (strcmp(child->GetName(), "View") == 0)
          {
          int port;
          // If port exists, then it must match the port number for this port.
          if (child->GetScalarAttribute("port", &port))
            {
            if (opPort->getPortNumber() != port)
              {
              continue;
              }
            }
          if (child->GetAttribute("type"))
            {
            return child->GetAttribute("type");
            }
          }
        else if (strcmp(child->GetName(), "OutputPort") == 0 &&
          child->GetAttribute("type") &&
          strcmp(child->GetAttribute("type"), "text") ==  0)
          {
          is_text = true;
          }
        }
      }
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

  vtkPVDataInformation* datainfo = opPort->getDataInformation();
  QString className = datainfo?  datainfo->GetDataClassName() : QString();

  // * Check if we should create the 2D view.
  if ((className == "vtkImageData" || className == "vtkUniformGrid") &&
    datainfo->GetCompositeDataClassName()==0)
    {
    int extent[6];
    datainfo->GetExtent(extent);
    int temp[6]={0, 0, 0, 0, 0, 0};
    int dimensionality = vtkStructuredData::GetDataDimension(
      vtkStructuredData::SetExtent(extent, temp));
    if (dimensionality == 2)
      {
      return pqTwoDRenderView::twoDRenderViewType();
      }
    }

  //Check if we should create any of the Plot Views.
  if (datainfo && ( className == "vtkRectilinearGrid" || className == "vtkImageData"  ) )
    {
    int extent[6];
    datainfo->GetExtent(extent);
    int temp[6]={0, 0, 0, 0, 0, 0};
    int dimensionality = vtkStructuredData::GetDataDimension(
      vtkStructuredData::SetExtent(extent, temp));

    vtkPVDataSetAttributesInformation* cellDataInfo =
      datainfo->GetCellDataInformation();
    vtkPVDataSetAttributesInformation* pointDataInfo =
      datainfo->GetPointDataInformation();
    if (dimensionality == 1 && cellDataInfo->GetNumberOfArrays() > 0)
      {
      // Has cell data, mostlikely this is a histogram.
      view_type = pqXYBarChartView::XYBarChartViewType();
      }
    else if (dimensionality == 1 &&
      (pointDataInfo->GetNumberOfArrays() > 0 ||
       cellDataInfo->GetNumberOfArrays() > 0 ) &&
      datainfo->GetNumberOfPoints() > 1)
      {
      // No cell data, but some point data -- may be a XY line plot.
      view_type = pqXYChartView::XYChartViewType();
      }
    }

  // Show table in spreadsheet view by default (unless the table is to be
  // treated as a "string" source).
  if (className == "vtkTable" && !is_text)
    {
    return pqSpreadSheetView::spreadsheetViewType(); 
    }

   // The proxy gives us no hint. In that case we try to determine the
  // preferred view by looking at the output from the source.
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
    if (currentView && currentView->getViewType() == view_type)
      {
      // nothing to do, active view is preferred view.
      }
    else
      {
      // if the currentView is empty (no visible representations), destroy it
      if(currentView && !currentView->getNumberOfVisibleRepresentations())
        {
        builder->destroy(currentView);
        }
      // Create the preferred view only if the current one is not of the same type
      // as the preferred view.
      currentView = builder->createView(view_type, opPort->getServer());
      }
    }

  if (!currentView || (currentView && !currentView->canDisplay(opPort)))
    {
    vtkPVDataInformation* info = opPort->getDataInformation();
    // GetDataSetType() == -1 signifies that there's no data to show.
    if (info->GetDataSetType() != -1)
      {
      // The user has selected a frame that is empty or the current view cannot
      // show the data and the the source does not
      // recommend any view type. Hence we create a render view.
      currentView = builder->createView(pqRenderView::renderViewType(),
        opPort->getServer());
      }
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

  return this->setRepresentationVisibility(opPort, view, true);
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
      repr = this->newRepresentation(opPort, view);
      }
    }
  if (!repr)
    {
    if (view && view->canDisplay(opPort))
      {
      qDebug() << "Cannot show the data in the current view although "
        "the view reported that it can show the data.";
      }
    return 0;
    }

  repr->setVisible(visible);

  // If this is the only source displayed in the view, reset the camera to make
  // sure its visible. Only do so if a source is being turned ON. Otherwise when
  // the next to last source is turned off, the camera would be reset to fit the
  // last remaining one which would be unexpected to the user
  // (hence the conditional on "visible")
  if(view->getNumberOfVisibleRepresentations()==1 && visible)
    {
    view->resetDisplay();
    }

  return repr;
}

//-----------------------------------------------------------------------------
pqDisplayPolicy::VisibilityState pqDisplayPolicy::getVisibility(
  pqView* view, pqOutputPort* port) const
{
  if (view && port)
    {
    pqDataRepresentation *repr = port->getRepresentation(view);
    if (repr && repr->isVisible())
      {
      // If repr for the view exists and is visible
      return Visible;
      }
    else if (repr || view->canDisplay(port))
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

  // Default behavior if no view is present
  return Hidden;
}


//-----------------------------------------------------------------------------
pqDataRepresentation* pqDisplayPolicy::newRepresentation(pqOutputPort* port,
  pqView* view) const
{
  return pqApplicationCore::instance()->getObjectBuilder()->
    createDataRepresentation(port, view);
}
