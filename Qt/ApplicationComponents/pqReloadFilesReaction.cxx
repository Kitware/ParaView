/*=========================================================================

   Program: ParaView
   Module:  pqReloadFilesReaction.cxx

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
#include "pqReloadFilesReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqPipelineSource.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "vtkNew.h"
#include "vtkSMReaderReloadHelper.h"
#include "vtkSMSourceProxy.h"

#include <QAbstractButton>
#include <QMessageBox>
#include <QtDebug>

namespace
{
bool PromptForNewFiles(vtkSMSourceProxy* reader)
{
  pqProxy* pqreader =
    pqApplicationCore::instance()->getServerManagerModel()->findItem<pqProxy*>(reader);
  if (pqreader && pqreader->property("pqReloadFilesReaction::CachedState").isValid())
  {
    return pqreader->property("pqReloadFilesReaction::CachedState").toBool();
  }

  QMessageBox mbox(QMessageBox::Question, QObject::tr("Reload Options"),
    QObject::tr("This reader supports file series. Do you want to look for new files "
                "in the series and load those, or reload the existing files?"),
    QMessageBox::Yes | QMessageBox::No, pqCoreUtilities::mainWidget());
  mbox.setObjectName("reloadOptionsMessageBox");
  mbox.button(QMessageBox::Yes)->setObjectName("findNewFilesButton");
  mbox.button(QMessageBox::Yes)->setText(QObject::tr("Find new files"));

  mbox.button(QMessageBox::No)->setObjectName("reloadExistingButton");
  mbox.button(QMessageBox::No)->setText(QObject::tr("Reload existing file(s)"));
  mbox.exec();

  bool retval = mbox.clickedButton() == mbox.button(QMessageBox::Yes);
  // cache response for future use.
  if (pqreader)
  {
    pqreader->setProperty("pqReloadFilesReaction::CachedState", retval);
  }
  return retval;
}
}

//-----------------------------------------------------------------------------
pqReloadFilesReaction::pqReloadFilesReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  this->connect(&pqActiveObjects::instance(), SIGNAL(sourceChanged(pqPipelineSource*)),
    SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
pqReloadFilesReaction::~pqReloadFilesReaction() = default;

//-----------------------------------------------------------------------------
void pqReloadFilesReaction::updateEnableState()
{
  vtkSMProxy* source = pqActiveObjects::instance().activeSource()
    ? pqActiveObjects::instance().activeSource()->getProxy()
    : nullptr;

  vtkNew<vtkSMReaderReloadHelper> helper;
  this->parentAction()->setEnabled(helper->SupportsReload(vtkSMSourceProxy::SafeDownCast(source)));
}

//-----------------------------------------------------------------------------
bool pqReloadFilesReaction::reload()
{
  vtkSMProxy* source = pqActiveObjects::instance().activeSource()
    ? pqActiveObjects::instance().activeSource()->getProxy()
    : nullptr;
  return pqReloadFilesReaction::reload(vtkSMSourceProxy::SafeDownCast(source));
}

//-----------------------------------------------------------------------------
bool pqReloadFilesReaction::reload(vtkSMSourceProxy* proxy)
{
  vtkNew<vtkSMReaderReloadHelper> helper;
  if (!helper->SupportsReload(proxy))
  {
    return false;
  }

  BEGIN_UNDO_EXCLUDE();
  if (helper->SupportsFileSeries(proxy) && PromptForNewFiles(proxy))
  {
    helper->ExtendFileSeries(proxy);
  }
  else
  {
    helper->ReloadFiles(proxy);
  }
  pqApplicationCore::instance()->render();
  END_UNDO_EXCLUDE();
  return true;
}
