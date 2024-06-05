// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqHelpWindow.h"
#include "pqApplicationCore.h"
#include "pqQtWidgetsConfig.h" // for PARAVIEW_USE_QTWEBENGINE
#include "pqSettings.h"
#include "ui_pqHelpWindow.h"

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

#include <cassert>

class pqBrowser
{
public:
  pqBrowser() = default;
  virtual ~pqBrowser() = default;
  virtual QWidget* widget() const = 0;
  virtual void setUrl(const QUrl& url) = 0;
  virtual QUrl url() = 0;
  virtual QUrl goBackward() = 0;
  virtual QUrl goForward() = 0;
  virtual bool canGoBackward() = 0;
  virtual bool canGoForward() = 0;

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
  QUrl url() override { return this->Widget->url(); }
  QUrl goBackward() override { return this->Widget->goBackward(); }
  QUrl goForward() override { return this->Widget->goForward(); }
  bool canGoBackward() override { return this->Widget->canGoBackward(); }
  bool canGoForward() override { return this->Widget->canGoForward(); }
};

#if PARAVIEW_USE_QTWEBENGINE
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
class pqHelpWindow::pqInternals : public Ui::pqHelpWindow
{
public:
  QString NameSpaceName;
};

//-----------------------------------------------------------------------------
pqHelpWindow::pqHelpWindow(QHelpEngine* engine, QWidget* parentObject, Qt::WindowFlags parentFlags)
  : Superclass(parentObject, parentFlags)
  , HelpEngine(engine)
  , Browser(new PQBROWSER_TYPE(this->HelpEngine, this))
  , Internals(new pqInternals())
{
  assert(engine != nullptr);

  this->Internals->setupUi(this);

  QObject::connect(this->HelpEngine, &QHelpEngine::warning, this, &pqHelpWindow::helpWarnings);

  this->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

  this->tabifyDockWidget(this->Internals->contentsDock, this->Internals->searchDock);
  this->Internals->contentsDock->setWidget(this->HelpEngine->contentWidget());
  this->Internals->contentsDock->raise();

  QWidget* searchPane = new QWidget(this);
  QVBoxLayout* vbox = new QVBoxLayout();
  searchPane->setLayout(vbox);
  vbox->addWidget(engine->searchEngine()->queryWidget());
  vbox->addWidget(engine->searchEngine()->resultWidget());
  this->Internals->searchDock->setWidget(searchPane);

  QObject::connect(engine->searchEngine()->queryWidget(), &QHelpSearchQueryWidget::search, this,
    &pqHelpWindow::search);
  QObject::connect(engine->searchEngine()->resultWidget(),
    &QHelpSearchResultWidget::requestShowLink, this,
    QOverload<const QUrl&>::of(&pqHelpWindow::showPage));

  this->setCentralWidget(this->Browser->widget());

  QObject::connect(
    this->Internals->actionHome, &QAction::triggered, this, [=]() { this->showHomePage(); });

  this->Internals->actionBackward->setEnabled(false);
  QObject::connect(
    this->Internals->actionBackward, &QAction::triggered, this, &pqHelpWindow::goBackward);

  this->Internals->actionForward->setEnabled(false);
  QObject::connect(
    this->Internals->actionForward, &QAction::triggered, this, &pqHelpWindow::goForward);

  QObject::connect(this->Internals->actionSaveAsHomepage, &QAction::triggered, this,
    &pqHelpWindow::saveCurrentHomePage);

  QWidget* spacer = new QWidget();
  spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  // toolBar is a pointer to an existing toolbar
  this->Internals->toolBar->insertWidget(this->Internals->actionSaveAsHomepage, spacer);

  QObject::connect(this->HelpEngine->contentWidget(), SIGNAL(linkActivated(const QUrl&)), this,
    SLOT(showPage(const QUrl&)));
}

//-----------------------------------------------------------------------------
pqHelpWindow::~pqHelpWindow() = default;

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
  QString query = this->HelpEngine->searchEngine()->queryWidget()->searchInput();
  this->HelpEngine->searchEngine()->search(query);
}

//-----------------------------------------------------------------------------
void pqHelpWindow::setNameSpace(const QString& namespace_name)
{
  this->Internals->NameSpaceName = namespace_name;
}

//-----------------------------------------------------------------------------
void pqHelpWindow::showHomePage()
{
  QUrl homePage;

  // First, search for valid saved homepage
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QString settingKey = QString("HelpWindow.%1.HomePage").arg(this->Internals->NameSpaceName);
  QUrl savedHomePage =
    settings->contains(settingKey) ? settings->value(settingKey).toUrl() : QUrl();

  if (!savedHomePage.isEmpty())
  {
    if (!this->HelpEngine->fileData(savedHomePage).isEmpty())
    {
      homePage = savedHomePage;
    }
    else
    {
      qWarning() << "Saved custom home page not found : " << savedHomePage
                 << ".\nDisplaying default home page instead.";
    }
  }

  // If no valid homepage is found, search for the default one
  if (homePage.isEmpty())
  {
    // Locate a file named index.html in the help engine registered files to use as homepage.
    QList<QUrl> html_pages =
      this->HelpEngine->files(this->Internals->NameSpaceName, QStringList(), "html");
    Q_FOREACH (QUrl url, html_pages)
    {
      if (url.path().endsWith("index.html"))
      {
        homePage = url;
      }
    }
  }

  if (homePage.isEmpty())
  {
    qWarning() << "Could not locate index.html";
    return;
  }

  this->showPage(homePage);
  QHelpContentWidget* widget = this->HelpEngine->contentWidget();
  widget->setCurrentIndex(widget->indexOf(homePage));
}

//-----------------------------------------------------------------------------
void pqHelpWindow::saveCurrentHomePage()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->setValue(
    QString("HelpWindow.%1.HomePage").arg(this->Internals->NameSpaceName), this->Browser->url());
}

//-----------------------------------------------------------------------------
void pqHelpWindow::goBackward()
{
  QUrl newUrl = this->Browser->goBackward();
  QHelpContentWidget* widget = this->HelpEngine->contentWidget();
  widget->setCurrentIndex(widget->indexOf(newUrl));
}

//-----------------------------------------------------------------------------
void pqHelpWindow::goForward()
{
  QUrl newUrl = this->Browser->goForward();
  QHelpContentWidget* widget = this->HelpEngine->contentWidget();
  widget->setCurrentIndex(widget->indexOf(newUrl));
}

//-----------------------------------------------------------------------------
void pqHelpWindow::updateHistoryButtons()
{
  this->Internals->actionBackward->setEnabled(this->Browser->canGoBackward());
  this->Internals->actionForward->setEnabled(this->Browser->canGoForward());
}
