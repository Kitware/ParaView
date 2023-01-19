/*=========================================================================

   Program: ParaView
   Module:  pqHelpWindow.h

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
#ifndef pqHelpWindow_h
#define pqHelpWindow_h

#include "pqWidgetsModule.h"
#include <QMainWindow>
#include <QScopedPointer>
#include <QUrl>

class QHelpEngine;
class pqBrowser;

/**
 * pqHelpWindow provides a assistant-like window  for showing help provided by
 * a QHelpEngine.
 */
class PQWIDGETS_EXPORT pqHelpWindow : public QMainWindow
{
  Q_OBJECT
  typedef QMainWindow Superclass;

public:
  pqHelpWindow(
    QHelpEngine* engine, QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{});
  ~pqHelpWindow() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Requests showing of a particular page. The url must begin with "qthelp:"
   * scheme when referring to pages from the help files.
   */
  virtual void showPage(const QString& url);
  virtual void showPage(const QUrl& url);

  /**
   * Tires to locate a file name index.html in the given namespace and then
   * shows that page.
   */
  virtual void showHomePage(const QString& namespace_name);

  ///@{
  /**
   * Navigate through the history of visited pages.
   */
  virtual void goToHomePage();
  virtual void goBackward();
  virtual void goForward();
  ///@}

  /**
   * Update the state of buttons used to navigate through history.
   */
  virtual void updateHistoryButtons();

Q_SIGNALS:
  /**
   * fired to relay warning messages from the help system.
   */
  void helpWarnings(const QString&);

protected Q_SLOTS:
  void search();

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Convenience method used to update the content widget (displaying all
   * available pages and the current selection) when navigating through history.
   */
  virtual void updateContentWidget(const QUrl& url);

  QHelpEngine* HelpEngine;
  QUrl HomePage;

private:
  Q_DISABLE_COPY(pqHelpWindow)
  const QScopedPointer<pqBrowser> Browser;

  class pqInternals;
  pqInternals* Internals;
};

#endif
