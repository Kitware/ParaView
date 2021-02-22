/*=========================================================================

   Program: ParaView
   Module:    pqLinkSelectionReaction.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#include "pqLinkSelectionReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqLinksModel.h"
#include "pqSelectionLinkDialog.h"
#include "pqSelectionManager.h"
#include "pqServerManagerModel.h"
#include "vtkSMSourceProxy.h"

#include <QSet>

//-----------------------------------------------------------------------------
pqLinkSelectionReaction::pqLinkSelectionReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(sourceChanged(pqPipelineSource*)), this,
    SLOT(updateEnableState()));

  // nameChanged() is fired even when modified state is changed ;).
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(modifiedStateChanged(pqServerManagerModelItem*)), this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqLinkSelectionReaction::updateEnableState()
{
  pqSelectionManager* selectionManager =
    qobject_cast<pqSelectionManager*>(pqApplicationCore::instance()->manager("SelectionManager"));
  if (selectionManager)
  {
    pqPipelineSource* activeSource = pqActiveObjects::instance().activeSource();
    if (activeSource != nullptr && selectionManager->hasActiveSelection())
    {
      foreach (pqOutputPort* port, selectionManager->getSelectedPorts())
      {
        if (port->getSource() == activeSource)
        {
          this->parentAction()->setEnabled(false);
          return;
        }
      }
      this->parentAction()->setEnabled(true);
      return;
    }
  }
  this->parentAction()->setEnabled(false);
}

//-----------------------------------------------------------------------------
void pqLinkSelectionReaction::linkSelection()
{
  pqSelectionLinkDialog slDialog(pqCoreUtilities::mainWidget());
  if (slDialog.exec() != QDialog::Accepted)
  {
    return;
  }

  pqSelectionManager* selectionManager =
    qobject_cast<pqSelectionManager*>(pqApplicationCore::instance()->manager("SelectionManager"));

  int index = 0;
  pqLinksModel* model = pqApplicationCore::instance()->getLinksModel();
  QString name = QString("SelectionLink%1").arg(index);
  while (model->getLink(name))
  {
    name = QString("SelectionLink%1").arg(++index);
  }

  pqPipelineSource* activeSource = pqActiveObjects::instance().activeSource();
  model->addSelectionLink(name, selectionManager->getSelectedPort()->getSourceProxy(),
    activeSource->getProxy(), slDialog.convertToIndices());
}
