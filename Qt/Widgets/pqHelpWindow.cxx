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

#include <cassert>

#include <QDebug>
#include <QDesktopServices>
#include <QHelpContentWidget>
#include <QHelpEngine>
#include <QHelpIndexWidget>
#include <QHelpSearchEngine>
#include <QHelpSearchQueryWidget>
#include <QHelpSearchResultWidget>
#include <QPointer>
#include <QUrl>

class pqBrowser
{
public:
  pqBrowser() {}
  virtual ~pqBrowser() {}
  virtual QWidget* widget() const = 0;
  virtual void setUrl(const QUrl& url) = 0;

private:
  Q_DISABLE_COPY(pqBrowser)
};

template <class T>
class pqBrowserTemplate : public pqBrowser
{
  QPointer<T> Widget;

public:
  pqBrowserTemplate(QHelpEngine* engine, pqHelpWindow* self)
  {
    this->Widget = T::newInstance(engine, self);
  }
  ~pqBrowserTemplate() override { delete this->Widget; }
  QWidget* widget() const override { return this->Widget; }
  void setUrl(const QUrl& url) override { this->Widget->setUrl(url); }
};

#ifdef PARAVIEW_USE_QTWEBENGINE
#include "pqHelpWindowWebEngine.h"
typedef pqBrowserTemplate<pqWebView> PQBROWSER_TYPE;
#else
#include "pqHelpWindowNoWebEngine.h"
typedef pqBrowserTemplate<pqTextBrowser> PQBROWSER_TYPE;
#endif

// ****************************************************************************
//            CLASS pqHelpWindow
// ****************************************************************************

//-----------------------------------------------------------------------------
pqHelpWindow::pqHelpWindow(QHelpEngine* engine, QWidget* parentObject, Qt::WindowFlags parentFlags)
  : Superclass(parentObject, parentFlags)
  , HelpEngine(engine)
  , Browser(new PQBROWSER_TYPE(this->HelpEngine, this))
{
  assert(engine != NULL);

  Ui::pqHelpWindow ui;
  ui.setupUi(this);

  QObject::connect(
    this->HelpEngine, SIGNAL(warning(const QString&)), this, SIGNAL(helpWarnings(const QString&)));

  this->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

  this->tabifyDockWidget(ui.contentsDock, ui.searchDock);
  ui.contentsDock->setWidget(this->HelpEngine->contentWidget());
  ui.contentsDock->raise();

  QWidget* searchPane = new QWidget(this);
  QVBoxLayout* vbox = new QVBoxLayout();
  searchPane->setLayout(vbox);
  vbox->addWidget(engine->searchEngine()->queryWidget());
  vbox->addWidget(engine->searchEngine()->resultWidget());
  ui.searchDock->setWidget(searchPane);

  QObject::connect(engine->searchEngine()->queryWidget(), SIGNAL(search()), this, SLOT(search()));
  QObject::connect(engine->searchEngine()->resultWidget(), SIGNAL(requestShowLink(const QUrl&)),
    this, SLOT(showPage(const QUrl&)));

  this->setCentralWidget(this->Browser->widget());

  QObject::connect(this->HelpEngine->contentWidget(), SIGNAL(linkActivated(const QUrl&)), this,
    SLOT(showPage(const QUrl&)));
}

//-----------------------------------------------------------------------------
pqHelpWindow::~pqHelpWindow()
{
}

//-----------------------------------------------------------------------------
void pqHelpWindow::showPage(const QString& url)
{
  this->showPage(QUrl(url));
}

//-----------------------------------------------------------------------------
void pqHelpWindow::showPage(const QUrl& url)
{
  if (url.scheme() == "http")
  {
    QDesktopServices::openUrl(url);
  }
  else
  {
    this->Browser->setUrl(url);
  }
}

//-----------------------------------------------------------------------------
void pqHelpWindow::search()
{
  QList<QHelpSearchQuery> query = this->HelpEngine->searchEngine()->queryWidget()->query();
  this->HelpEngine->searchEngine()->search(query);
}

//-----------------------------------------------------------------------------
void pqHelpWindow::showHomePage(const QString& namespace_name)
{
  QList<QUrl> html_pages = this->HelpEngine->files(namespace_name, QStringList(), "html");
  // now try to locate a file named index.html in this collection.
  foreach (QUrl url, html_pages)
  {
    if (url.path().endsWith("index.html"))
    {
      this->showPage(url);
      return;
    }
  }
  qWarning() << "Could not locate index.html";
}
