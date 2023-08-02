// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqApplyPropertiesReaction_h
#define pqApplyPropertiesReaction_h

#include "pqApplicationComponentsModule.h"

#include "pqReaction.h"

#include <QPointer>

class pqPropertiesPanel;

class PQAPPLICATIONCOMPONENTS_EXPORT pqApplyPropertiesReaction : public pqReaction
{
  typedef pqReaction Superclass;

public:
  pqApplyPropertiesReaction(pqPropertiesPanel* panel, QAction* action, bool apply);

  void updateEnableState() override;

protected:
  void onTriggered() override;

private:
  QPointer<pqPropertiesPanel> Panel;
  bool ShouldApply;
};
#endif
