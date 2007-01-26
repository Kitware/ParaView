/*=========================================================================

   Program: ParaView
   Module:    pqObjectInspectorDriver.cxx

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

/// \file pqObjectInspectorDriver.cxx
/// \date 1/12/2007

#include "pqObjectInspectorDriver.h"

#include "pqGenericViewModule.h"
#include "pqConsumerDisplay.h"
#include "pqPipelineSource.h"
#include "pqProxy.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerModelItem.h"
#include "pqServerManagerSelectionModel.h"


pqObjectInspectorDriver::pqObjectInspectorDriver(QObject *parentObject)
  : QObject(parentObject)
{
  this->Selection = 0;
  this->Source = 0;
  this->Display = 0;
  this->View = 0;
  this->ShowCurrent = true;
}

void pqObjectInspectorDriver::setSelectionModel(
    pqServerManagerSelectionModel *model)
{
  if(this->Selection == model)
    {
    return;
    }

  if(this->Selection)
    {
    this->disconnect(this->Selection, 0, this, 0);
    this->disconnect(this->Selection->model(), 0, this, 0);
    }

  this->Selection = model;
  if(this->Selection)
    {
    this->connect(this->Selection,
        SIGNAL(currentChanged(pqServerManagerModelItem *)),
        this, SLOT(updateSource()));
    this->connect(this->Selection,
        SIGNAL(selectionChanged(const pqServerManagerSelection &, const pqServerManagerSelection &)),
        this, SLOT(updateSource()));
    this->connect(this->Selection->model(),
        SIGNAL(preSourceRemoved(pqPipelineSource *)),
        this, SLOT(checkSource(pqPipelineSource *)));
    }
}

void pqObjectInspectorDriver::setActiveView(pqGenericViewModule *view)
{
  if(view != this->View)
    {
    this->View = view;
    this->checkForDisplay();
    }
}

void pqObjectInspectorDriver::updateSource()
{
  this->setActiveSource(this->findSource());
}

void pqObjectInspectorDriver::checkSource(pqPipelineSource *source)
{
  if(source && source == this->Source)
    {
    this->setActiveSource(source);
    }
}

void pqObjectInspectorDriver::checkForDisplay()
{
  pqConsumerDisplay *display = this->findDisplay();
  if(display != this->Display)
    {
    this->Display = display;
    emit this->displayChanged(this->Display, this->View);
    }
}

void pqObjectInspectorDriver::checkDisplay(pqPipelineSource *,
    pqConsumerDisplay *display)
{
  if(display && display == this->Display)
    {
    this->Display = 0;
    emit this->displayChanged(this->Display, this->View);
    }
}

void pqObjectInspectorDriver::setActiveSource(pqPipelineSource *source)
{
  if(source == this->Source)
    {
    return;
    }

  if(this->Source)
    {
    this->disconnect(this->Source, 0, this, 0);
    }

  this->Source = source;
  if(this->Source)
    {
    this->connect(this->Source,
        SIGNAL(displayAdded(pqPipelineSource *, pqConsumerDisplay *)),
        this, SLOT(checkForDisplay()), Qt::QueuedConnection);
    this->connect(this->Source,
        SIGNAL(displayRemoved(pqPipelineSource *, pqConsumerDisplay *)),
        this, SLOT(checkDisplay(pqPipelineSource *, pqConsumerDisplay *)));
    }

  emit this->sourceChanged(this->Source);

  // Update the active display.
  this->checkForDisplay();
}

pqPipelineSource *pqObjectInspectorDriver::findSource() const
{
  pqServerManagerModelItem *item = 0;
  const pqServerManagerSelection *selected = this->Selection->selectedItems();
  if(selected->size() == 1)
    {
    item = selected->first();
    }
  else if(selected->size() > 1 && this->ShowCurrent)
    {
    item = this->Selection->currentItem();
    if(item && !this->Selection->isSelected(item))
      {
      item = 0;
      }
    }

  return dynamic_cast<pqPipelineSource *>(item);
}

pqConsumerDisplay *pqObjectInspectorDriver::findDisplay() const
{
  if(this->Source && this->View)
    {
    return this->Source->getDisplay(this->View);
    }

  return 0;
}


