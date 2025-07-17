// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqMainWindowEventBehavior.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqLoadDataReaction.h"
#include "pqLoadStateReaction.h"
#include "pqMainWindowEventManager.h"
#include "pqSaveStateReaction.h"
#include "pqSettings.h"

#include "vtkSMLoadStateOptionsProxy.h"

#include <QApplication>
#include <QDropEvent>
#include <QMessageBox>
#include <QMimeData>
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
pqMainWindowEventBehavior::~pqMainWindowEventBehavior() = default;

//-----------------------------------------------------------------------------
void pqMainWindowEventBehavior::onClose(QCloseEvent* event)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  if (core->settings()->value("GeneralSettings.ShowSaveStateOnExit", false).toBool())
  {
    QString title = tr("Exit %1").arg(QApplication::applicationName());
    QString message =
      tr("Do you want to save the state before exiting %1?").arg(QApplication::applicationName());
    auto choice = QMessageBox::question(qobject_cast<QWidget*>(sender()), title, message,
      QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    switch (choice)
    {
      case QMessageBox::Save:
        pqSaveStateReaction::saveState();
        return;
      case QMessageBox::Cancel:
        event->ignore();
        return;
      case QMessageBox::Discard:
      default:
        return;
    }
  }
}

//-----------------------------------------------------------------------------
void pqMainWindowEventBehavior::onShow(QShowEvent*) {}

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

  QStringList files;

  Q_FOREACH (QUrl url, urls)
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
        std::string contents;
        const bool statePresentInPNF = path.endsWith(".png", Qt::CaseInsensitive) &&
          vtkSMLoadStateOptionsProxy::PNGHasStateFile(path.toStdString().c_str(), contents);
        if (statePresentInPNF &&
          QMessageBox::warning(pqCoreUtilities::mainWidget(), tr("Read PNG or state file?"),
            tr("This PNG file has a ParaView state file embedded.\n"
               "Do you want to open this file as a state file?"),
            QMessageBox::No | QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
        {
          pqLoadStateReaction::loadState(path);
        }
        else
        {
          files.append(url.toLocalFile());
        }
      }
    }
  }

  // If we have no file we return
  if (files.empty() || files.first().isEmpty())
  {
    return;
  }
  pqLoadDataReaction::loadFilesForSupportedTypes({ files });
}
