// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqReloadFilesReaction_h
#define pqReloadFilesReaction_h

#include "pqReaction.h"
class vtkSMSourceProxy;

/**
 * @ingroup Reactions
 *
 * pqReloadFilesReaction adds handler code to reload the active reader.
 * It calls the "reload" property, identified by hints, if present, or calls
 * vtkSMProxy::RecreateVTKObjects().
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqReloadFilesReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqReloadFilesReaction(QAction* parent = nullptr);
  ~pqReloadFilesReaction() override;

  /**
   * reload the active proxy if it supports reload. Returns true on success.
   */
  static bool reload();

  /**
   * reload the \c proxy if it supports reload. Returns true on success.
   */
  static bool reload(vtkSMSourceProxy* proxy);

protected:
  void onTriggered() override { this->reload(); }
  void updateEnableState() override;

private:
  Q_DISABLE_COPY(pqReloadFilesReaction)
};

#endif
