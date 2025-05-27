// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqReloadFilesReaction_h
#define pqReloadFilesReaction_h

#include "pqReaction.h"
#include "vtkParaViewDeprecation.h"

class vtkSMSourceProxy;

/**
 * @ingroup Reactions
 *
 * pqReloadFilesReaction adds handler code to reload the active reader or all readers
 * depending on the ReloadMode passed in the constructor. It calls the "reload" property,
 * identified by hints, if present, or calls vtkSMProxy::RecreateVTKObjects().
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqReloadFilesReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  enum ReloadModes
  {
    ActiveSource,
    AllSources
  };

  pqReloadFilesReaction(QAction* parent = nullptr, ReloadModes reloadMode = ActiveSource);
  ~pqReloadFilesReaction() override;

  /**
   * reload the active proxy if it supports reload. Returns true on success.
   */
  PARAVIEW_DEPRECATED_IN_6_0_0("Use reloadSources() instead.")
  static bool reload();

  /**
   * reload the active source or all sources if they support reload. Returns true on success.
   */
  bool reloadSources();

  /**
   * reload the \c proxy if it supports reload. Returns true on success.
   */
  static bool reload(vtkSMSourceProxy* proxy);

protected:
  void onTriggered() override { this->reloadSources(); }
  void updateEnableState() override;

private:
  Q_DISABLE_COPY(pqReloadFilesReaction)

  ReloadModes ReloadMode;
};

#endif
