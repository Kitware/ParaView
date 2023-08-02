// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqMainWindowEventBehavior_h
#define pqMainWindowEventBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

class QCloseEvent;
class QDragEnterEvent;
class QDropEvent;
class QShowEvent;

/**
 * @ingroup Reactions
 * Reaction to when things are dragged into the main window.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqMainWindowEventBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqMainWindowEventBehavior(QObject* parent = nullptr);
  ~pqMainWindowEventBehavior() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Triggered when a close event occurs on the main window.
   */
  void onClose(QCloseEvent*);

  /**
   * Triggered when a show event occurs on the main window.
   */
  void onShow(QShowEvent*);

  /**
   * Triggered when a drag enter event occurs on the main window.
   */
  void onDragEnter(QDragEnterEvent*);

  /**
   * Triggered when a drop event occurs on the main window.
   */
  void onDrop(QDropEvent*);

private:
  Q_DISABLE_COPY(pqMainWindowEventBehavior)
};

#endif
