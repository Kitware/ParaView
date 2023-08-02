// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqHelpWindowNoWebEngine_h
#define pqHelpWindowNoWebEngine_h

/**
 *============================================================================
 * This is an internal header used by pqHelpWindow.
 * This header gets included when PARAVIEW_USE_QTWEBENGINE is OFF.
 *============================================================================
 */

#include <QTextBrowser>

namespace
{
// ****************************************************************************
//            CLASS pqTextBrowser
// ****************************************************************************
/**
 * Internal class used to add overload the QTextBrowser
 */
class pqTextBrowser : public QTextBrowser
{
public:
  pqTextBrowser(QHelpEngine* helpEngine, QWidget* _parent = nullptr)
  {
    this->HelpEngine = helpEngine;
    this->setParent(_parent);
    this->setOpenLinks(false);
  }

  ~pqTextBrowser() override = default;
  static pqTextBrowser* newInstance(QHelpEngine* engine, pqHelpWindow* self)
  {
    pqTextBrowser* instance = new pqTextBrowser(engine, self);
    self->connect(instance, &pqTextBrowser::anchorClicked, self,
      QOverload<const QUrl&>::of(&pqHelpWindow::showPage));
    self->connect(
      instance, &pqTextBrowser::historyChanged, self, &pqHelpWindow::updateHistoryButtons);
    return instance;
  }

  void setUrl(const QUrl& url) { this->setSource(url); }

  QUrl url() { return this->source(); }

  QUrl goBackward()
  {
    this->backward();
    return this->source();
  }

  QUrl goForward()
  {
    this->forward();
    return this->source();
  }

  bool canGoBackward() { return this->isBackwardAvailable(); }

  bool canGoForward() { return this->isForwardAvailable(); }

protected:
  /**
   * Implementation reference from:
   * http://doc.qt.digia.com/qq/qq28-qthelp.html
   */
  QVariant loadResource(int type, const QUrl& url) override
  {
    if (url.scheme() == "qthelp")
    {
      return QVariant(this->HelpEngine->fileData(url));
    }
    else
    {
      return QTextBrowser::loadResource(type, url);
    }
  }
  QPointer<QHelpEngine> HelpEngine;
};

} // end of namespace

#endif
