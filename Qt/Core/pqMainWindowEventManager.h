// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqMainWindowEventManager_h
#define pqMainWindowEventManager_h

#include "pqCoreModule.h"
#include <QObject>

class QCloseEvent;
class QDragEnterEvent;
class QShowEvent;
class QDropEvent;

/**
 * pqMainWindowEventManager is a manager for marshalling a main window's events
 * to pqReactions.
 *
 * ParaView and ParaView-derived applications have their own main window class
 * that inherits from QMainWindow. The standard Qt approach to intercepting
 * signals to the main window (e.g. "close application") is to reimplement
 * QMainWindow's event methods (e.g. closeEvent(QCloseEvent*)). This polymorphic
 * solution is not available to ParaView-derived applications comprised of
 * plugins, however. To facilitate a plugin's ability to influence the behavior
 * of ParaView's main window, pqMainWindowEventManager is a registered manager
 * that emits signals when the main window receives events.
 */
class PQCORE_EXPORT pqMainWindowEventManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqMainWindowEventManager(QObject* p = nullptr);
  ~pqMainWindowEventManager() override;

  /**
   * notify when a close event has been sent to the main window.
   */
  void closeEvent(QCloseEvent*);

  /**
   * notify when a show event has been sent to the main window.
   */
  void showEvent(QShowEvent*);

  /**
   * notify when a drag enter event has been sent to the main window.
   */
  void dragEnterEvent(QDragEnterEvent*);

  /**
   * notify when a drop event has been sent to the main window.
   */
  void dropEvent(QDropEvent*);

Q_SIGNALS:
  /**
   * notification when a close event has been sent to the main window.
   */
  void close(QCloseEvent*);

  /**
   * notification when a show event has been sent to the main window.
   */
  void show(QShowEvent*);

  /**
   * notification when a drag enter event has been sent to the main window.
   */
  void dragEnter(QDragEnterEvent*);

  /**
   * notification when a drop event has been sent to the main window.
   */
  void drop(QDropEvent*);
};

#endif
