/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqCopyReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqUndoStack.h"
#include "vtkNew.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyClipboard.h"
//-----------------------------------------------------------------------------
pqCopyReaction::pqCopyReaction(QAction* parentObject, bool paste_mode)
  : Superclass(parentObject)
  , Paste(paste_mode)
{
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(sourceChanged(pqPipelineSource*)), this,
    SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
pqCopyReaction::~pqCopyReaction() = default;

//-----------------------------------------------------------------------------
void pqCopyReaction::updateEnableState()
{
  if (this->Paste)
  {
    QObject* clipboard = pqApplicationCore::instance()->manager("SOURCE_ON_CLIPBOARD");
    pqPipelineSource* active = pqActiveObjects::instance().activeSource();
    this->parentAction()->setEnabled(
      clipboard != nullptr && active != clipboard && active != nullptr);
  }
  else
  {
    this->parentAction()->setEnabled(pqActiveObjects::instance().activeSource() != nullptr);
  }
}

//-----------------------------------------------------------------------------
void pqCopyReaction::copy()
{
  pqPipelineSource* activeSource = pqActiveObjects::instance().activeSource();
  if (!activeSource)
  {
    qDebug("Could not find an active source to copy to.");
    return;
  }

  // since pqApplicationCore uses QPointer for the managers, we don't have to
  // worry about unregistering the source when it is deleted.
  pqApplicationCore* appCore = pqApplicationCore::instance();
  // need to remove any previous SOURCE_ON_CLIPBOARD else pqApplicationCore
  // warns.
  appCore->unRegisterManager("SOURCE_ON_CLIPBOARD");
  appCore->registerManager("SOURCE_ON_CLIPBOARD", activeSource);
}

//-----------------------------------------------------------------------------
void pqCopyReaction::paste()
{
  pqPipelineSource* activeSource = pqActiveObjects::instance().activeSource();
  pqPipelineSource* clipboard =
    qobject_cast<pqPipelineSource*>(pqApplicationCore::instance()->manager("SOURCE_ON_CLIPBOARD"));
  if (!clipboard)
  {
    qDebug("No source on clipboard to copy from.");
    return;
  }
  pqCopyReaction::copy(activeSource->getProxy(), clipboard->getProxy());
  activeSource->renderAllViews();
}

//-----------------------------------------------------------------------------
void pqCopyReaction::copy(vtkSMProxy* dest, vtkSMProxy* source)
{
  if (dest && source)
  {
    BEGIN_UNDO_SET("Copy Properties");

    vtkNew<vtkSMProxyClipboard> clipboard;
    clipboard->Copy(source);
    clipboard->Paste(dest);

    END_UNDO_SET();
  }
}
