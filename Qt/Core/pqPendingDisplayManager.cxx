/*=========================================================================

   Program: ParaView
   Module:    pqPendingDisplayManager.cxx

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


#include "pqPendingDisplayManager.h"

// Qt includes
#include <QPointer>
#include <QtDebug>

// Server Manager includes
#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"

// pq includes
#include "pqApplicationCore.h"
#include "pqConsumerDisplay.h"
#include "pqGenericViewModule.h"
#include "pqPendingDisplayUndoElement.h"
#include "pqPipelineBuilder.h"
#include "pqPipelineFilter.h"
#include "pqRenderViewModule.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "pqDisplayPolicy.h"

//-----------------------------------------------------------------------------
class pqPendingDisplayManager::MyInternal
{
public:
  QList<QPointer< pqPipelineSource> > SourcesSansDisplays;
};

//-----------------------------------------------------------------------------
pqPendingDisplayManager::pqPendingDisplayManager(QObject* p)
  : QObject(p)
{
  this->Internal = new MyInternal;
}

//-----------------------------------------------------------------------------
pqPendingDisplayManager::~pqPendingDisplayManager()
{
  delete this->Internal;
}


//-----------------------------------------------------------------------------
void pqPendingDisplayManager::addPendingDisplayForSource(pqPipelineSource* s)
{
  if(!s)
    {
    return;
    }

  this->internalAddPendingDisplayForSource(s);

  pqPendingDisplayUndoElement* elem = pqPendingDisplayUndoElement::New();
  elem->PendingDisplay(s, true);
  pqApplicationCore::instance()->getUndoStack()->AddToActiveUndoSet(elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
void pqPendingDisplayManager::internalAddPendingDisplayForSource(pqPipelineSource* s)
{
  if (!this->Internal->SourcesSansDisplays.contains(s))
    {
    this->Internal->SourcesSansDisplays.push_back(s);

    if(this->Internal->SourcesSansDisplays.size() == 1)
      {
      emit this->pendingDisplays(true);
      }
    }
}

//-----------------------------------------------------------------------------
void pqPendingDisplayManager::removePendingDisplayForSource(pqPipelineSource* s)
{
  if (this->Internal->SourcesSansDisplays.contains(s))
    {
    this->Internal->SourcesSansDisplays.removeAll(s);
    
    if(this->Internal->SourcesSansDisplays.size() == 0)
      {
      emit this->pendingDisplays(false);
      }
    }
}

//-----------------------------------------------------------------------------
void pqPendingDisplayManager::createPendingDisplays(
  pqGenericViewModule* activeview)
{
  pqDisplayPolicy* displayPolicy = 
    pqApplicationCore::instance()->getDisplayPolicy();
  if (!displayPolicy)
    {
    qCritical() << "No display policy defined. Cannot create pending displays.";
    return;
    }

  foreach (pqPipelineSource* source, this->Internal->SourcesSansDisplays)
    {
    if (!source)
      {
      continue;
      }

    pqDisplay* display = displayPolicy->createPreferredDisplay(
      source, activeview, false);
    if (!display || display->getNumberOfViewModules() != 1)
      {
      continue;
      }

    pqGenericViewModule* view = display->getViewModule(0); 
    pqRenderViewModule* renModule = qobject_cast<pqRenderViewModule*>(view);
    if (renModule && renModule->getDisplayCount() == 1)
      {
      renModule->resetCamera();
      renModule->resetCenterOfRotation();
      }

    pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(source);
    if (filter && filter->replaceInput())
      {
      // hide input source.
      QList<pqPipelineSource*> inputs = filter->getInputs();
      for(int cc=0; cc < inputs.size(); ++cc)
        {
        pqPipelineSource* input_src = inputs[cc];
        pqConsumerDisplay* disp = input_src->getDisplay(view);
        if (disp)
          {
          disp->setVisible(false);
          }
        }
      }
    view->render();

    // For every pending display we create, we push an undoelement
    // nothing the creation of the pending display. 
    // This ensures that when this step is undone, the source for
    // which we created the pending display is once again marked as
    // a source pending a display.
    pqPendingDisplayUndoElement* elem = pqPendingDisplayUndoElement::New();
    elem->PendingDisplay(source, false);
    pqApplicationCore::instance()->getUndoStack()->AddToActiveUndoSet(elem);
    elem->Delete();
    }

  this->Internal->SourcesSansDisplays.clear();
  emit this->pendingDisplays(false);
}

//-----------------------------------------------------------------------------
int pqPendingDisplayManager::getNumberOfPendingDisplays()
{
  return this->Internal->SourcesSansDisplays.size();
}

