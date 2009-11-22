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
#include <QDir>
#include <QPointer>
#include <QStringList>

#include "pqCoreUtilities.h"
#include "pqHelpWindow.h"

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

  // * Discover help project files from the resources.
  QDir dir(QString(":/%1/Documentation").arg(QApplication::applicationName()));
  QStringList help_files;
  if (dir.exists())
    {
    QStringList filters;
    filters << "*.qch";
    help_files = dir.entryList(filters, QDir::Files);
    }
  if (help_files.size() == 0)
    {
    qWarning() << "No Qt compressed help file (*.qch) was located.";
    return;
    }

  QString file = 
    QString(":/%1/Documentation/%2").arg(QApplication::applicationName()).arg(help_files[0]);
  helpWindow = new pqHelpWindow(
    QString("%1 Online Help").arg(QApplication::applicationName()),
    pqCoreUtilities::mainWidget());
  QString namespace_name = helpWindow->registerDocumentation(file);

  help_files.pop_front();
  foreach (file, help_files)
    {
    helpWindow->registerDocumentation(file);
    }

  helpWindow->showHomePage(namespace_name);
  helpWindow->show();
  helpWindow->raise();
  if (!url.isEmpty())
    {
    helpWindow->showPage(url);
    }
}

