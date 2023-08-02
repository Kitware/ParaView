// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLoadMaterialsReaction_h
#define pqLoadMaterialsReaction_h

#include "pqReaction.h"

/**
 * @class pqLoadMaterialsReaction
 * @ingroup Reactions
 * @brief reaction to import an ospray material definition file
 *
 * pqLoadMaterialsReaction is a reaction to import an file containing
 * ospray material definitions
 */
class pqServer;

class PQAPPLICATIONCOMPONENTS_EXPORT pqLoadMaterialsReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqLoadMaterialsReaction(QAction* parent);
  ~pqLoadMaterialsReaction() override;

  static bool loadMaterials();
  static bool loadMaterials(const QString& dbase, pqServer* server = nullptr);

protected:
  /// Called when the action is triggered.
  void onTriggered() override { this->loadMaterials(); }

private:
  Q_DISABLE_COPY(pqLoadMaterialsReaction)
};

#endif
