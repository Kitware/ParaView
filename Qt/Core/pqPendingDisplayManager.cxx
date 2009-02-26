/*=========================================================================

   Program: ParaView
   Module:    pqPendingDisplayManager.cxx

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


#include "pqPendingDisplayManager.h"

// Qt includes
#include <QPointer>
#include <QtDebug>

// Server Manager includes
#include "vtkPVXMLElement.h"
#include "vtkSMPVRepresentationProxy.h"

// pq includes
#include "pqApplicationCore.h"
#include "pqDisplayPolicy.h"
#include "pqOutputPort.h"
#include "pqPendingDisplayUndoElement.h"
#include "pqPipelineFilter.h"
#include "pqPipelineRepresentation.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"

//-----------------------------------------------------------------------------
class pqPendingDisplayManager::MyInternal
{
public:
  QList<QPointer< pqPipelineSource> > SourcesSansDisplays;
  QPointer<pqUndoStack> UndoStack;
  QPointer<pqView> ActiveView;
};

//-----------------------------------------------------------------------------
pqPendingDisplayManager::pqPendingDisplayManager(QObject* p)
  : QObject(p)
{
  this->Internal = new MyInternal;
  this->IgnoreAdd = false;
  
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
      SIGNAL(preSourceRemoved(pqPipelineSource*)),
      this, 
      SLOT(removePendingDisplayForSource(pqPipelineSource*)));
}

//-----------------------------------------------------------------------------
pqPendingDisplayManager::~pqPendingDisplayManager()
{
  delete this->Internal;
}


//-----------------------------------------------------------------------------
void pqPendingDisplayManager::setUndoStack(pqUndoStack* stack)
{
  this->Internal->UndoStack = stack;
}

//-----------------------------------------------------------------------------
pqUndoStack* pqPendingDisplayManager::undoStack() const
{
  return this->Internal->UndoStack;
}

//-----------------------------------------------------------------------------
void pqPendingDisplayManager::setAddSourceIgnored(bool ignored)
{
  this->IgnoreAdd = ignored;
}

//-----------------------------------------------------------------------------
bool pqPendingDisplayManager::isPendingDisplay(pqPipelineSource* source) const
{
  return (source && this->Internal->SourcesSansDisplays.contains(source));
}

//-----------------------------------------------------------------------------
void pqPendingDisplayManager::addPendingDisplayForSource(pqPipelineSource* s)
{
  if(!s || this->IgnoreAdd)
    {
    return;
    }

  this->internalAddPendingDisplayForSource(s);

  if (this->Internal->UndoStack)
    {
    pqPendingDisplayUndoElement* elem = pqPendingDisplayUndoElement::New();
    elem->PendingDisplay(s, true);
    this->Internal->UndoStack->addToActiveUndoSet(elem);
    elem->Delete();
    }
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
  pqView* activeview)
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

    // Create representations for all output ports.
    for (int cc=0; cc < source->getNumberOfOutputPorts(); cc++)
      {
      pqDataRepresentation* repr = displayPolicy->createPreferredRepresentation(
        source->getOutputPort(cc), activeview, false);
      if (!repr || !repr->getView())
        {
        continue;
        }

      pqView* view = repr->getView(); 
      pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(source);
      if (filter)
        {
        int replace_input = filter->replaceInput();
        if (replace_input > 0)
          {
          // hide input source.
          QList<pqOutputPort*> inputs = filter->getAllInputs();
          for(int kk=0; kk < inputs.size(); ++kk)
            {
            pqOutputPort* input = inputs[kk];
            pqDataRepresentation* inputRepr = input->getRepresentation(view);
            if (inputRepr)
              {
              pqPipelineRepresentation* sourceDisp =
                qobject_cast<pqPipelineRepresentation*>(inputRepr);
              if (sourceDisp && replace_input == 2)
                {
                // Conditionally turn off the input. The input should be turned
                // off if the representation is surface and the opacity is 1.
                int reprType = sourceDisp->getRepresentationType();
                if ((reprType != vtkSMPVRepresentationProxy::SURFACE &&
                    reprType != vtkSMPVRepresentationProxy::SURFACE_WITH_EDGES) ||
                  sourceDisp->getOpacity() < 1.0)
                  {
                  continue;
                  }
                }
              inputRepr->setVisible(false);
              }
            }
          }
        }
      view->render(); // these renders are collapsed.
      }

    if (this->Internal->UndoStack)
      {
      // For every pending repr we create, we push an undoelement
      // nothing the creation of the pending repr. 
      // This ensures that when this step is undone, the source for
      // which we created the pending repr is once again marked as
      // a source pending a repr.
      pqPendingDisplayUndoElement* elem = pqPendingDisplayUndoElement::New();
      elem->PendingDisplay(source, false);
      this->Internal->UndoStack->addToActiveUndoSet(elem);
      elem->Delete();
      }
    }

  this->Internal->SourcesSansDisplays.clear();
  emit this->pendingDisplays(false);
}

//-----------------------------------------------------------------------------
int pqPendingDisplayManager::getNumberOfPendingDisplays()
{
  return this->Internal->SourcesSansDisplays.size();
}

//-----------------------------------------------------------------------------
void pqPendingDisplayManager::setActiveView(pqView* view)
{
  this->Internal->ActiveView = view;
}

//-----------------------------------------------------------------------------
void pqPendingDisplayManager::createPendingDisplays()
{
  this->createPendingDisplays(this->Internal->ActiveView);
}
