// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqHelpReaction.h"

#include <QApplication>
#include <QDebug>
#include <QHelpEngine>
#include <QPointer>
#include <QStringList>

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqHelpWindow.h"
#include "pqPluginDocumentationBehavior.h"

//-----------------------------------------------------------------------------
pqHelpReaction::pqHelpReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
void pqHelpReaction::showHelp()
{
  pqHelpReaction::showHelp(QString());
}

//-----------------------------------------------------------------------------
void pqHelpReaction::showHelp(const QString& url)
{
  static QPointer<pqHelpWindow> helpWindow;
  if (helpWindow)
  {
    // raise assistant window;
    helpWindow->show();
    helpWindow->raise();
    if (!url.isEmpty())
    {
      helpWindow->showPage(url);
    }
    return;
  }

  QHelpEngine* engine = pqApplicationCore::instance()->helpEngine();
  new pqPluginDocumentationBehavior(engine);

  helpWindow = new pqHelpWindow(engine, pqCoreUtilities::mainWidget());
  helpWindow->setWindowTitle(tr("%1 Online Help").arg(QApplication::applicationName()));

  // show some home page. Pick the first registered documentation and show its
  // home page.
  QStringList registeredDocumentations = engine->registeredDocumentations();
  if (!registeredDocumentations.empty())
  {
    helpWindow->setNameSpace(registeredDocumentations[0]);
    helpWindow->showHomePage();
  }
  helpWindow->show();
  helpWindow->raise();
  if (!url.isEmpty())
  {
    helpWindow->showPage(url);
  }
}

//-----------------------------------------------------------------------------
void pqHelpReaction::showProxyHelp(const QString& group, const QString& name)
{
  // initializes the help engine.
  pqHelpReaction::showHelp();

  QHelpEngine* engine = pqApplicationCore::instance()->helpEngine();

  // now determine the url for this proxy.
  Q_FOREACH (const QString& doc_namespace, engine->registeredDocumentations())
  {
    QString basename = QFileInfo(doc_namespace).baseName();
    QString url =
      QString("qthelp://%1/%2/%3.%4.html").arg(doc_namespace).arg(basename).arg(group).arg(name);

    // If URL actually point to an existing file
    if (!engine->fileData(url).isEmpty())
    {
      pqHelpReaction::showHelp(url);
      break;
    }
  }
}
