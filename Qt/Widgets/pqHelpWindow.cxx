/*=========================================================================

   Program: ParaView
   Module:    pqHelpWindow.cxx

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
#include "pqHelpWindow.h"
#include "ui_pqHelpWindow.h"

#include <QApplication>
#include <QHelpEngine>
#include <QHelpContentWidget>
#include <QHelpContentModel>
#include <QHelpContentItem>
#include <QHelpIndexWidget>
#include <QDir>
#include <QTextBrowser>
#include <QUrl>
#include <QTemporaryFile>
#include <QLibraryInfo>
#include <QtDebug>

class pqHelpWindow::pqTextBrowser : public QTextBrowser
{
public:
  pqTextBrowser(QHelpEngine* engine, QWidget * parentObject = 0)
    : QTextBrowser(parentObject)
    {
    Q_ASSERT(engine != 0);
    this->Engine = engine;
    }

  virtual QVariant loadResource(int type, const QUrl &url)
    {
    if (url.scheme() == "qthelp")
      {
      return QVariant(this->Engine->fileData(url));
      }
    else
      {
      return QTextBrowser::loadResource(type, url);
      }
    }
private:
  QHelpEngine* Engine;
};

//-----------------------------------------------------------------------------
pqHelpWindow::pqHelpWindow(
  const QString& wtitle, QWidget* parentObject, Qt::WindowFlags parentFlags)
  : Superclass(parentObject, parentFlags)
{
  Ui::pqHelpWindow ui;
  ui.setupUi(this);

  this->setWindowTitle(wtitle);

  QTemporaryFile tFile;
  tFile.open();
  this->HelpEngine = new QHelpEngine(tFile.fileName() + ".qhc", this);

  QObject::connect(this->HelpEngine, SIGNAL(warning(const QString&)),
    this, SIGNAL(helpWarnings(const QString&)));

  this->HelpEngine->setupData();

  ui.contentsDock->setWidget(this->HelpEngine->contentWidget());
  ui.indexDock->setWidget(this->HelpEngine->indexWidget());
  ui.indexDock->hide();

  pqHelpWindow::pqTextBrowser* browser = 
    new pqHelpWindow::pqTextBrowser(this->HelpEngine, this);
  this->Browser = browser;
  this->setCentralWidget(browser);
  QObject::connect(
    this->HelpEngine->contentWidget(), SIGNAL(linkActivated(const QUrl&)),
    browser, SLOT(setSource(const QUrl&)));
}

//-----------------------------------------------------------------------------
pqHelpWindow::~pqHelpWindow()
{
  QString collectionFile = this->HelpEngine->collectionFile();
  delete this->HelpEngine;
  QFile::remove(collectionFile);
}

//-----------------------------------------------------------------------------
void pqHelpWindow::showPage(const QString& url)
{
  this->Browser->setSource(url);
}

//-----------------------------------------------------------------------------
void pqHelpWindow::showHomePage(const QString& namespace_name)
{
  QList<QUrl> html_pages = this->HelpEngine->files(namespace_name,
    QStringList(), "html");
  // now try to locate a file named index.html in this collection.
  foreach (QUrl url, html_pages)
    {
    if (url.path().endsWith("index.html"))
      {
      this->showPage(url.toString());
      return;
      }
    }
  qWarning() << "Could not locate index.html";
}

//-----------------------------------------------------------------------------
QString pqHelpWindow::registerDocumentation(const QString& qchfilename)
{
  QString filename = qchfilename;
  // this piece of code handles the case where a resource file name is passed.
  QFile file(qchfilename);
  QTemporaryFile *tFile =QTemporaryFile::createLocalFile(file);
  if (tFile)
    {
    filename = tFile->fileName();
    tFile->setParent(this);
    tFile->setAutoRemove(true);
    }
  this->HelpEngine->registerDocumentation(filename);
  return this->HelpEngine->namespaceName(filename);
}

