/*=========================================================================

   Program: ParaView
   Module:    pqHelpReaction.cxx

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
  helpWindow->setWindowTitle(QString("%1 Online Help").arg(QApplication::applicationName()));

  // show some home page. Pick the first registered documentation and show its
  // home page.
  QStringList registeredDocumentations = engine->registeredDocumentations();
  if (registeredDocumentations.size() > 0)
  {
    helpWindow->showHomePage(registeredDocumentations[0]);
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
  foreach (const QString& doc_namespace, engine->registeredDocumentations())
  {
    QString basename = QFileInfo(doc_namespace).baseName();
    QString url =
      QString("qthelp://%1/%2/%3.%4.html").arg(doc_namespace).arg(basename).arg(group).arg(name);
    if (engine->findFile(url).isValid())
    {
      pqHelpReaction::showHelp(url);
    }
  }
}
