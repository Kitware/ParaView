/*=========================================================================

   Program:   ParaQ
   Module:    pqPipelineSource.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

/// \file pqPipelineSource.cxx
/// \date 4/17/2006

#include "pqPipelineSource.h"

// ParaView.
#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkSMSourceProxy.h"

// Qt
#include <QList>
#include <QtDebug>

// ParaQ
#include "pqObjectPanel.h"
#include "pqPipelineBuilder.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineLink.h"
#include "pqPipelineFilter.h"
#include "pqProcessModuleGUIHelper.h"
#include "pqPropertyManager.h"
#include "pqMainWindow.h"

//-----------------------------------------------------------------------------
class pqPipelineSourceInternal 
{
public:
  vtkSmartPointer<vtkSMProxy> Proxy;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;

  QString Name;
  QList<pqPipelineSource*> Outputs;
  QList<pqPipelineDisplay*> Displays;

  pqPipelineSourceInternal(QString name, vtkSMProxy* proxy)
    {
    this->Name = name;
    this->Proxy = proxy;
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    }
};


//-----------------------------------------------------------------------------
pqPipelineSource::pqPipelineSource(QString name, vtkSMProxy* proxy,
  pqServer* server, QObject* parent/*=NULL*/) : pqPipelineObject(server, parent)
{
  this->Internal = new pqPipelineSourceInternal(name, proxy);
  
  // Set the model item type.
  this->setType(pqPipelineModel::Source);
 
  if (proxy) // TODO: clean this.
    {
    QObject::connect(&pqObjectPanel::PropertyManager, SIGNAL(accepted()),
      this, SLOT(onFirstAccept()));
    }
}

//-----------------------------------------------------------------------------
pqPipelineSource::~pqPipelineSource()
{
  if (this->Internal->Proxy.GetPointer())
    {
    this->Internal->VTKConnect->Disconnect(this->Internal->Proxy.GetPointer());
    }

  delete this->Internal;
}

//-----------------------------------------------------------------------------
const QString& pqPipelineSource::getProxyName() const 
{
  return this->Internal->Name;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqPipelineSource::getProxy() const
{
  return this->Internal->Proxy.GetPointer();
}

//-----------------------------------------------------------------------------
int pqPipelineSource::getOutputCount() const
{
  return this->Internal->Outputs.size();
}

//-----------------------------------------------------------------------------
pqPipelineSource *pqPipelineSource::getOutput(int index) const
{
  if(index >= 0 && index < this->Internal->Outputs.size())
    {
    return this->Internal->Outputs[index];
    }

  qCritical() << "Index " << index << " out of bounds.";
  return 0;
}

//-----------------------------------------------------------------------------
int pqPipelineSource::getOutputIndexFor(pqPipelineSource *output) const
{
  int index = 0;
  if(output)
    {
    pqPipelineLink *link = 0;
    foreach(pqPipelineSource *current, this->Internal->Outputs)
      {
      // The output may be in a link object.
      if(current->getType() == pqPipelineModel::Link)
        {
        link = dynamic_cast<pqPipelineLink *>(current);
        current = link->GetLink();
        }
      // The requested output object may be a link, so test the
      // list item as well as the link output.
      if(current == output)
        {
        return index;
        }
      index++;
      }
    }
  return -1;
}

//-----------------------------------------------------------------------------
bool pqPipelineSource::hasOutput(pqPipelineSource *output) const
{
  return (this->getOutputIndexFor(output) != -1);
}

//-----------------------------------------------------------------------------
void pqPipelineSource::onFirstAccept()
{
  /* FIXME:: this will have to move out of here...
  if (this->getDisplay()->GetDisplayCount() ==0)
    {
    this->setupDisplays();
    }
    */
  QObject::disconnect(this, SLOT(onFirstAccept()));
}

//-----------------------------------------------------------------------------
void pqPipelineSource::setupDisplays()
{
  /*
  // TODO: how to get the pqMainWindow instance more gracefully?
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pqProcessModuleGUIHelper* helper = 
    pqProcessModuleGUIHelper::SafeDownCast(pm->GetGUIHelper());
  if (!helper)
    {
    qCritical() << "Failed to locate GUIHelper.";
    return;
    }
  pqMainWindow* mainWindow = helper->getWindow();
  pqPipelineBuilder* pBuilder = mainWindow->pipelineBuilder();
  vtkSMRenderModuleProxy* renModule = mainWindow->activeRenderModule();
  if (renModule)
    {
    vtkSMDisplayProxy* display = pBuilder->createDisplayProxy(
      vtkSMSourceProxy::SafeDownCast(this->Internal->Proxy.GetPointer()), renModule);
    // the display will get registered by pBuilder and will get added
    // to this->Display as a side effect of the registeration. So nothing mucj
    // to do here.
    }
    */
}

//-----------------------------------------------------------------------------
void pqPipelineSource::addOutput(pqPipelineSource* output)
{
  this->Internal->Outputs.push_back(output);

  // raise signals to let the world know which connections were
  // broken and which ones were made.
  emit this->connectionAdded(this, output);
}

//-----------------------------------------------------------------------------
void pqPipelineSource::removeOutput(pqPipelineSource* output)
{
  int index = this->Internal->Outputs.indexOf(output);
  if (index != -1)
    {
    this->Internal->Outputs.removeAt(index);
    }

  // raise signals to let the world know which connections were
  // broken and which ones were made.
  emit this->connectionRemoved(this, output, index);
}

//-----------------------------------------------------------------------------
void pqPipelineSource::addDisplay(pqPipelineDisplay* display)
{
  this->Internal->Displays.push_back(display);

//  emit this->displayAdded(this, display);
}

//-----------------------------------------------------------------------------
void pqPipelineSource::removeDisplay(pqPipelineDisplay* display)
{
  int index = this->Internal->Displays.indexOf(display);
  if (index != -1)
    {
    this->Internal->Displays.removeAt(index);
    }
}

//-----------------------------------------------------------------------------
int pqPipelineSource::getDisplayCount() const
{
  return this->Internal->Displays.size();
}

//-----------------------------------------------------------------------------
pqPipelineDisplay* pqPipelineSource::getDisplay(int index) const
{
  if (index >= 0 && index < this->Internal->Displays.size())
    {
    return this->Internal->Displays[index];
    }
  return 0;
}
