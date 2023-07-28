// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqHelpWindow_h
#define pqHelpWindow_h

#include "vtkParaViewDeprecation.h" // For deprecation

#include "pqComponentsModule.h" // For export macro

#include <QMainWindow>
#include <QScopedPointer>
#include <QUrl>

class QHelpEngine;
class pqBrowser;

/**
 * pqHelpWindow provides a assistant-like window  for showing help provided by
 * a QHelpEngine.
 */
class PQCOMPONENTS_EXPORT pqHelpWindow : public QMainWindow
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
   * Tries to locate a file name index.html in the current namespace and then
   * shows that page.
   */
  virtual void showHomePage();

  /**
   * Set the namespace from the provided one, then tries to locate a file name
   * index.html in the given namespace and shows that page.
   */
  PARAVIEW_DEPRECATED_IN_5_12_0(
    "Please use separated setNameSpace() and showHomePage() functions instead.")
  virtual void showHomePage(const QString& namespace_name);

  /**
   * Save current page as home page.
   * This parameter is saved in a setting.
   */
  virtual void saveCurrentHomePage();

  ///@{
  /**
   * Navigate through the history of visited pages.
   */
  virtual void goBackward();
  virtual void goForward();
  ///@}

  /**
   * Update the state of buttons used to navigate through history.
   */
  virtual void updateHistoryButtons();

  /**
   * Sets the namespace to use in order to find the homepage.
   */
  virtual void setNameSpace(const QString& namespace_name);

Q_SIGNALS:
  /**
   * fired to relay warning messages from the help system.
   */
  void helpWarnings(const QString&);

protected Q_SLOTS:
  void search();

protected: // NOLINT(readability-redundant-access-specifiers)
  QHelpEngine* HelpEngine;

private:
  Q_DISABLE_COPY(pqHelpWindow)
  const QScopedPointer<pqBrowser> Browser;

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
