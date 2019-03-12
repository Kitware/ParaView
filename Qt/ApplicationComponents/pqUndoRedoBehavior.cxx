/*=========================================================================

   Program: ParaView
   Module:    pqUndoRedoBehavior.cxx

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
#include "pqUndoRedoBehavior.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "pqUndoStackBuilder.h"

#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"

#include <QDebug>

//-----------------------------------------------------------------------------
pqUndoRedoBehavior::pqUndoRedoBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  if (core->getUndoStack())
  {
    qCritical() << "Application wide undo-stack has already been initialized.";
    return;
  }

  // setup Undo Stack.
  pqUndoStackBuilder* builder = pqUndoStackBuilder::New();
  pqUndoStack* stack = new pqUndoStack(builder, this);
  vtkSMProxyManager::GetProxyManager()->SetUndoStackBuilder(builder);
  builder->Delete();
  core->setUndoStack(stack);

  // clear undo stack when state is loaded.
  QObject::connect(
    core, SIGNAL(stateLoaded(vtkPVXMLElement*, vtkSMProxyLocator*)), stack, SLOT(clear()));

  // clear stack when server connects/disconnects.
  QObject::connect(
    core->getServerManagerModel(), SIGNAL(serverAdded(pqServer*)), stack, SLOT(clear()));
  QObject::connect(
    core->getServerManagerModel(), SIGNAL(finishedRemovingServer()), stack, SLOT(clear()));
}
