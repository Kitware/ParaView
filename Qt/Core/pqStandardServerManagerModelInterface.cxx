/*=========================================================================

   Program: ParaView
   Module:    pqStandardServerManagerModelInterface.cxx

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

========================================================================*/
#include "pqStandardServerManagerModelInterface.h"

// Server Manager Includes.
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMPVRepresentationProxy.h"
// Qt Includes.
#include <QtDebug>

// ParaView Includes.
#include "pqAnimationCue.h"
#include "pqAnimationScene.h"
#include "pqApplicationCore.h"
#include "pqPipelineFilter.h"
#include "pqPipelineRepresentation.h"
#include "pqPluginManager.h"
#include "pqRenderView.h"
#include "pqScalarBarRepresentation.h"
#include "pqScalarsToColors.h"
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

  pqPluginManager* pluginMgr = pqApplicationCore::instance()->getPluginManager();
  if (group == "view_modules")
    {
    QObjectList ifaces = pluginMgr->interfaces();
    foreach(QObject* iface, ifaces)
      {
      pqViewModuleInterface* vmi = qobject_cast<pqViewModuleInterface*>(iface);
      if(vmi && vmi->viewTypes().contains(xml_type))
        {
        return vmi->createView(xml_type, group, name, 
          vtkSMViewProxy::SafeDownCast(proxy), server, 0);
        }
      }
    if (proxy->IsA("vtkSMRenderViewProxy"))
      {
      return new pqRenderView(group, name, 
        vtkSMRenderViewProxy::SafeDownCast(proxy), server, 0);
      }
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
  else if (group == "scalar_bars")
    {
    return new pqScalarBarRepresentation(group, name, proxy, server, 0);
    }
  else if (group == "displays")
    {
    QObjectList ifaces = pluginMgr->interfaces();
    foreach(QObject* iface, ifaces)
      {
      pqViewModuleInterface* vmi = qobject_cast<pqViewModuleInterface*>(iface);
      if(vmi && vmi->displayTypes().contains(xml_type))
        {
        return vmi->createDisplay(xml_type, "displays", name, proxy, server, 0);
        }
      }
    if (proxy->IsA("vtkSMPVRepresentationProxy"))
      {
      return new pqPipelineRepresentation(group, name, 
        vtkSMPVRepresentationProxy::SafeDownCast(proxy), server, 0);
      }
    if (proxy->IsA("vtkSMDataRepresentationProxy"))
      {
      // If everything fails, simply create a pqDataRepresentation object.
      return new pqDataRepresentation(group, name, proxy, server, 0);
      }
    }
  else if (group == "animation")
    {
    if (proxy->IsA("vtkSMPVAnimationSceneProxy"))
      {
      return new pqAnimationScene(group, name, proxy, server, 0);
      }
    else if (proxy->IsA("vtkSMAnimationCueProxy"))
      {
      return new pqAnimationCue(group, name, proxy, server, 0);
      }
    }

  // qDebug() << "Could not determine pqProxy type: " << proxy->GetXMLName() << endl;
  return 0;
}


