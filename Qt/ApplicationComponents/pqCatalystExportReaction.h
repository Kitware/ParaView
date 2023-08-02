// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCatalystExportReaction_h
#define pqCatalystExportReaction_h

#include "pqApplicationComponentsModule.h"
#include "pqReaction.h"

#include "vtkType.h" // For vtkTypeUInt32

/**
 * @ingroup Reactions
 * Reaction to export a Catalyst script that will produce configured catalyst data products.
 */

class PQAPPLICATIONCOMPONENTS_EXPORT pqCatalystExportReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqCatalystExportReaction(QAction* parent);
  ~pqCatalystExportReaction() override;

  /**
   * Export a Catalyst script. Returns true on success.
   */
  static bool exportScript(
    const QString& name, vtkTypeUInt32 location = 0x10 /*vtkPVSession::CLIENT*/);

  /**
   * Export a Catalyst script. Returns the non-empty name of the file
   * written on success.
   */
  static QString exportScript();

protected Q_SLOTS:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { this->exportScript(); }

private:
  Q_DISABLE_COPY(pqCatalystExportReaction)
};

#endif
