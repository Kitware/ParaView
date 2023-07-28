// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqReaction_h
#define pqReaction_h

#include "pqApplicationComponentsModule.h"
#include <QAction>
#include <QObject>

/**
 * @defgroup Reactions ParaView Reactions
 * ParaView client relies on a collection of reactions that are autonomous
 * entities that use pqApplicationCore and other core components to get their
 * work done. To use, simply attach an instance of pqReaction subclass to a
 * QAction. The reaction then monitors events from the QAction. Additionally, the
 * reaction can monitor the ParaView application state to update things like
 * enable state, label, etc. for the QAction itself.
 */

/**
 * @ingroup Reactions
 * This is a superclass just to make it easier to collect all such reactions.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqReaction : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  pqReaction(QAction* parent, Qt::ConnectionType type = Qt::AutoConnection);
  ~pqReaction() override;

  /**
   * Provides access to the parent action.
   */
  QAction* parentAction() const { return qobject_cast<QAction*>(this->parent()); }

protected Q_SLOTS:
  /**
   * Called when the action is triggered.
   */
  virtual void onTriggered() {}

  virtual void updateEnableState() {}
  virtual void updateMasterEnableState(bool);

protected: // NOLINT(readability-redundant-access-specifiers)
  bool IsMaster;

private:
  Q_DISABLE_COPY(pqReaction)
};

#endif
