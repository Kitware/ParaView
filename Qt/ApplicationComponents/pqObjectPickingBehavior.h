// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqObjectPickingBehavior_h
#define pqObjectPickingBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>
#include <QPoint>

class pqView;
class pqOutputPort;

/**
 * @ingroup Behaviors
 * pqObjectPickingBehavior is used to add support for picking "source" by
 * clicking on it in a view. This currently only supports render-view. But we
 * can add support for other views as needed.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqObjectPickingBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqObjectPickingBehavior(QObject* parent = nullptr);
  ~pqObjectPickingBehavior() override;

protected:
  /**
   * event filter to capture the left-click.
   */
  bool eventFilter(QObject* caller, QEvent* e) override;

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Called when a new view is added. We install event-filter to get click
   * events.
   */
  void onViewAdded(pqView*);

private:
  Q_DISABLE_COPY(pqObjectPickingBehavior)
  QPoint Position;
};

#endif
