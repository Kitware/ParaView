// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqEditCameraReaction_h
#define pqEditCameraReaction_h

#include "pqReaction.h"
#include <QPointer>

class pqView;

/**
 * @ingroup Reactions
 * pqEditCameraReaction is a reaction to show the edit-camera dialog.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqEditCameraReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqEditCameraReaction(QAction* parent, pqView* view = nullptr);

  /**
   * Shows the dialog for the view.
   */
  static void editCamera(pqView*);
public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;

private:
  Q_DISABLE_COPY(pqEditCameraReaction)
  QPointer<pqView> View;
};

#endif
