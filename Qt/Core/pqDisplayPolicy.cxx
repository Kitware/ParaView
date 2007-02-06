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

#include "vtkSMAbstractDisplayProxy.h"
#include "vtkSMAbstractViewModuleProxy.h"
#include "vtkSMProxyManager.h"

#include <QtDebug>
#include <QString>

#include "pqPipelineSource.h"
#include "pqGenericViewModule.h"
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

  // FIXME: Alternatively, we may want to check the source output
  // data type to decide whether the output can be plotted.
  QString viewProxyName = view->getProxy()->GetXMLName();
  QString srcProxyName = source->getProxy()->GetXMLName();

  if (viewProxyName == "BarChartViewModule")
    {
    return (srcProxyName == "ExtractHistogram");
    }

  if (viewProxyName == "XYPlotViewModule")
    {
    return (srcProxyName == "Probe2");
    }

  return true;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqDisplayPolicy::newDisplay(
  const pqPipelineSource* source, const pqGenericViewModule* view) const
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


