/*=========================================================================

   Program: ParaView
   Module:    pqStandardServerManagerModelInterface.cxx

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

========================================================================*/
#include "pqStandardServerManagerModelInterface.h"

// Server Manager Includes.
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
//#include "vtkSMScatterPlotRepresentationProxy.h"
// Qt Includes.
#include <QtDebug>

// ParaView Includes.
#include "pqAnimationCue.h"
#include "pqAnimationScene.h"
#include "pqApplicationCore.h"
#include "pqInterfaceTracker.h"
#include "pqPipelineFilter.h"
#include "pqPipelineRepresentation.h"
#include "pqRenderView.h"
#include "pqScalarBarRepresentation.h"
#include "pqScalarsToColors.h"
#include "pqScalarOpacityFunction.h"
//#include "pqScatterPlotRepresentation.h"
#include "pqTimeKeeper.h"
#include "pqViewModuleInterface.h"

//-----------------------------------------------------------------------------
pqStandardServerManagerModelInterface::pqStandardServerManagerModelInterface(
  QObject* _parent) : QObject(_parent)
{
}

//-----------------------------------------------------------------------------
pqStandardServerManagerModelInterface::~pqStandardServerManagerModelInterface()
{
}

//-----------------------------------------------------------------------------
pqProxy* pqStandardServerManagerModelInterface::createPQProxy(
  const QString& group, const QString& name, vtkSMProxy* proxy, pqServer* server) const
{
  QString xml_type = proxy->GetXMLName();

  pqInterfaceTracker* pluginMgr =
    pqApplicationCore::instance()->interfaceTracker();
  if (group == "views")
    {
    QObjectList ifaces = pluginMgr->interfaces();
    foreach(QObject* iface, ifaces)
      {
      pqViewModuleInterface* vmi = qobject_cast<pqViewModuleInterface*>(iface);
      if (vmi)
        {
        pqView* pqview = vmi->createView(xml_type, group, name, 
          vtkSMViewProxy::SafeDownCast(proxy), server, 0);
        if (pqview)
          {
          return pqview;
          }
        }
      }
    }
  else if (group == "layouts")
    {
    return new pqProxy(group, name, proxy, server, NULL);
    }
  else if (group == "sources")
    {
    if (proxy->GetProperty("Input"))
      {
      return new pqPipelineFilter(name, proxy, server, 0);
      }
    else
      {
      return new pqPipelineSource(name, proxy, server, 0);
      }
    }
  else if (group == "timekeeper")
    {
    return new pqTimeKeeper(group, name, proxy, server, 0);
    }
  else if (group == "lookup_tables")
    {
    return new pqScalarsToColors(group, name, proxy, server, 0);
    }
  else if (group == "piecewise_functions")
    {
    return new pqScalarOpacityFunction(group, name, proxy, server, 0);
    }
  else if (group == "scalar_bars")
    {
    return new pqScalarBarRepresentation(group, name, proxy, server, 0);
    }
  else if (group == "representations")
    {
    QObjectList ifaces = pluginMgr->interfaces();
    foreach(QObject* iface, ifaces)
      {
      pqViewModuleInterface* vmi = qobject_cast<pqViewModuleInterface*>(iface);
      if(vmi && vmi->displayTypes().contains(xml_type))
        {
        return vmi->createDisplay(
          xml_type, "representations", name, proxy, server, 0);
        }
      }
//    if (proxy->IsA("vtkSMScatterPlotRepresentationProxy"))
//      {
//      return new pqScatterPlotRepresentation(group, name,
//        vtkSMScatterPlotRepresentationProxy::SafeDownCast(proxy), server, 0);
//      }
    if (proxy->IsA("vtkSMRepresentationProxy") && proxy->GetProperty("Input"))
      {
      if (proxy->IsA("vtkSMPVRepresentationProxy") ||
        xml_type == "ImageSliceRepresentation")
        {
        // pqPipelineRepresentation is a design flaw! We need to get rid of it
        // and have helper code that manages the crap in that class
        return new pqPipelineRepresentation(group, name, proxy, server, 0);
        }
      // If everything fails, simply create a pqDataRepresentation object.
      return new pqDataRepresentation(group, name, proxy, server, 0);
      }
    }
  else if (group == "animation")
    {
    if (xml_type == "AnimationScene")
      {
      return new pqAnimationScene(group, name, proxy, server, 0);
      }
    else if (xml_type == "KeyFrameAnimationCue" ||
      xml_type == "CameraAnimationCue" ||
      xml_type == "TimeAnimationCue" ||
      xml_type == "PythonAnimationCue")
      {
      return new pqAnimationCue(group, name, proxy, server, 0);
      }
    }

  // qDebug() << "Could not determine pqProxy type: " << proxy->GetXMLName() << endl;
  return 0;
}


