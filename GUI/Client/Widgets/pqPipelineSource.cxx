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
#include "vtkSmartPointer.h"
#include "vtkSMProxy.h"

// Qt
#include <QList>
#include <QtDebug>

// ParaQ
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"

//-----------------------------------------------------------------------------
class pqPipelineSourceInternal 
{
public:
  vtkSmartPointer<vtkSMProxy> Proxy;

  QString Name;
  QList<pqPipelineSource*> Consumers;
  QList<pqPipelineDisplay*> Displays;

  pqPipelineSourceInternal(QString name, vtkSMProxy* proxy)
    {
    this->Name = name;
    this->Proxy = proxy;
    }
};


//-----------------------------------------------------------------------------
pqPipelineSource::pqPipelineSource(const QString& name, vtkSMProxy* proxy,
  pqServer* server, QObject* _parent/*=NULL*/) 
: pqProxy("sources", name, proxy, server, _parent)
{
  this->Internal = new pqPipelineSourceInternal(name, proxy);
  
  // Set the model item type.
  this->setType(pqPipelineModel::Source);
}

//-----------------------------------------------------------------------------
pqPipelineSource::~pqPipelineSource()
{
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
int pqPipelineSource::getNumberOfConsumers() const
{
  return this->Internal->Consumers.size();
}

//-----------------------------------------------------------------------------
pqPipelineSource *pqPipelineSource::getConsumer(int index) const
{
  if(index >= 0 && index < this->Internal->Consumers.size())
    {
    return this->Internal->Consumers[index];
    }

  qCritical() << "Index " << index << " out of bounds.";
  return 0;
}

//-----------------------------------------------------------------------------
int pqPipelineSource::getConsumerIndexFor(pqPipelineSource *consumer) const
{
  int index = 0;
  if(consumer)
    {
    foreach(pqPipelineSource *current, this->Internal->Consumers)
      {
      if(current == consumer)
        {
        return index;
        }
      index++;
      }
    }
  return -1;
}

//-----------------------------------------------------------------------------
bool pqPipelineSource::hasConsumer(pqPipelineSource *consumer) const
{
  return (this->getConsumerIndexFor(consumer) != -1);
}

//-----------------------------------------------------------------------------
void pqPipelineSource::addConsumer(pqPipelineSource* cons)
{
  this->Internal->Consumers.push_back(cons);

  // raise signals to let the world know which connections were
  // broken and which ones were made.
  emit this->connectionAdded(this, cons);
}

//-----------------------------------------------------------------------------
void pqPipelineSource::removeConsumer(pqPipelineSource* cons)
{
  int index = this->Internal->Consumers.indexOf(cons);
  if (index != -1)
    {
    this->Internal->Consumers.removeAt(index);
    }

  // raise signals to let the world know which connections were
  // broken and which ones were made.
  emit this->connectionRemoved(this, cons);
}

//-----------------------------------------------------------------------------
void pqPipelineSource::addDisplay(pqPipelineDisplay* display)
{
  this->Internal->Displays.push_back(display);

  emit this->displayAdded(this, display);
}

//-----------------------------------------------------------------------------
void pqPipelineSource::removeDisplay(pqPipelineDisplay* display)
{
  int index = this->Internal->Displays.indexOf(display);
  if (index != -1)
    {
    this->Internal->Displays.removeAt(index);
    }
  emit this->displayRemoved(this, display);
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
