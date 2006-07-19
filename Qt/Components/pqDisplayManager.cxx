/*=========================================================================

   Program: ParaView
   Module:    pqDisplayManager.cxx

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

#include "pqDisplayManager.h"

#include <QPointer>
#include <QMap>

#include "pqPipelineSource.h"
#include "pqPipelineBuilder.h"
#include "pqServerManagerModel.h"
#include "pqRenderModule.h"
#include "pqApplicationCore.h"
#include "pqPendingDisplayUndoElement.h"
#include "pqUndoStack.h"

typedef QMap< QPointer<pqPipelineSource>, QPointer<pqRenderModule> > DisplayMap;

class pqDisplayManager::pqInternal
{
public:
  DisplayMap SourcesSansDisplays;
};

pqDisplayManager::pqDisplayManager(QObject* p)
 : QObject(p)
{
  this->Internal = new pqInternal;
}


pqDisplayManager::~pqDisplayManager()
{
  delete this->Internal;
}

void pqDisplayManager::addDisplayForSource(pqPipelineSource* source,
                                           pqRenderModule* rm)
{
  pqApplicationCore::instance()->getPipelineBuilder()->createDisplayProxy(source,
    rm);
  renderModule->render();
  
  // reset the camera for the first one  (multiview ???)
  if (pqApplicationCore::instance()->getServerManagerModel()->getNumberOfSources() == 1)
    {
    renderModule->resetCamera();
    }
}

void pqDisplayManager::addDeferredDisplayForSource(pqPipelineSource* source, 
                                                   pqRenderModule* rm)
{
  this->Internal->SourcesSansDisplays.insert(source, rm);
}

void pqDisplayManager::createDeferredDisplays()
{
  QList<pqRenderModule*> renderModules;

  DisplayMap::iterator iter;
  for(iter = this->Internal->SourcesSansDisplays.begin();
      iter != this->Internal->SourcesSansDisplays.end();
      ++iter)
    {
    pqPipelineSource* source = iter.key();
    pqRenderModule* renderModule = iter.value();
    
    if (!source || !renderModule)
      {
      continue;
      }

    pqApplicationCore::instance()->getPipelineBuilder()->createDisplayProxy(source,
      renderModule);
    renderModule->render();
    
    // reset the camera for the first one  (multiview ???)
    if (pqApplicationCore::instance()->getServerManagerModel()->getNumberOfSources() == 1)
      {
      renderModule->resetCamera();
      }
    
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
}


