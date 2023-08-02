// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSaveDataReaction_h
#define pqSaveDataReaction_h

#include "pqReaction.h"

class pqPipelineSource;
class vtkPVDataInformation;

/**
 * @ingroup Reactions
 * Reaction to save data files.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqSaveDataReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  pqSaveDataReaction(QAction* parent);

  /**
   * Save data files from active port. Users the vtkSMWriterFactory to decide
   * what writes are available. Returns true if the creation is
   * successful, otherwise returns false.
   * Note that this method is static. Applications can simply use this without
   * having to create a reaction instance.
   */
  static bool saveActiveData(const QString& files);
  static bool saveActiveData();

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;
  /**
   * Triggered when a source became valid
   */
  void dataUpdated(pqPipelineSource* source);

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqSaveDataReaction::saveActiveData(); }

private:
  Q_DISABLE_COPY(pqSaveDataReaction)

  static QString defaultExtension(vtkPVDataInformation* info);
  static void setDefaultExtension(vtkPVDataInformation* info, const QString& ext);
};

#endif
