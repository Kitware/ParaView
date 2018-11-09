/*=========================================================================

   Program: ParaView
   Module:    pqMainWindowEventBehavior.cxx

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
#include "pqMainWindowEventBehavior.h"

#include "pqApplicationCore.h"
#include "pqLoadDataReaction.h"
#include "pqLoadStateReaction.h"
#include "pqMainWindowEventManager.h"
#include "pqSaveStateReaction.h"
#include "pqSettings.h"
#include "pqTimer.h"
#include "pqWelcomeDialog.h"

#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMessageBox>
#include <QMimeData>
#include <QShowEvent>
#include <QString>
#include <QUrl>

//-----------------------------------------------------------------------------
pqMainWindowEventBehavior::pqMainWindowEventBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  pqMainWindowEventManager* mainWindowEventManager =
    pqApplicationCore::instance()->getMainWindowEventManager();

  QObject::connect(
    mainWindowEventManager, SIGNAL(close(QCloseEvent*)), this, SLOT(onClose(QCloseEvent*)));

  QObject::connect(
    mainWindowEventManager, SIGNAL(show(QShowEvent*)), this, SLOT(onShow(QShowEvent*)));

  QObject::connect(mainWindowEventManager, SIGNAL(dragEnter(QDragEnterEvent*)), this,
    SLOT(onDragEnter(QDragEnterEvent*)));

  QObject::connect(
    mainWindowEventManager, SIGNAL(drop(QDropEvent*)), this, SLOT(onDrop(QDropEvent*)));
}

//-----------------------------------------------------------------------------
pqMainWindowEventBehavior::~pqMainWindowEventBehavior()
{
}

//-----------------------------------------------------------------------------
void pqMainWindowEventBehavior::onClose(QCloseEvent* event)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  if (core->settings()->value("GeneralSettings.ShowSaveStateOnExit", false).toBool())
  {
    if (QMessageBox::question(qobject_cast<QWidget*>(sender()), "Exit ParaView?",
          "Do you want to save the state before exiting ParaView?",
          QMessageBox::Save | QMessageBox::Discard) == QMessageBox::Save)
    {
      pqSaveStateReaction::saveState();
    }
  }
  event->accept();
}

//-----------------------------------------------------------------------------
void pqMainWindowEventBehavior::onShow(QShowEvent*)
{
}

//-----------------------------------------------------------------------------
void pqMainWindowEventBehavior::onDragEnter(QDragEnterEvent* event)
{
  event->acceptProposedAction();
}

//-----------------------------------------------------------------------------
void pqMainWindowEventBehavior::onDrop(QDropEvent* event)
{
  QList<QUrl> urls = event->mimeData()->urls();
  if (urls.isEmpty())
  {
    return;
  }

  QList<QString> files;

  foreach (QUrl url, urls)
  {
    if (!url.toLocalFile().isEmpty())
    {
      QString path = url.toLocalFile();
      if (path.endsWith(".pvsm", Qt::CaseInsensitive))
      {
        pqLoadStateReaction::loadState(path);
      }
      else
      {
        files.append(url.toLocalFile());
      }
    }
  }

  // If we have no file we return
  if (files.empty() || files.first().isEmpty())
  {
    return;
  }
  pqLoadDataReaction::loadData(files);
}
