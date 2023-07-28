// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSpreadsheetLoadDataReaction_h
#define pqSpreadsheetLoadDataReaction_h

#include <pqLoadDataReaction.h>

/**
 * A Load data reaction loading only .vtp, .pvd and .vtk.
 */
class pqSpreadsheetLoadDataReaction : public pqLoadDataReaction
{
  Q_OBJECT
  typedef pqLoadDataReaction Superclass;

public:
  pqSpreadsheetLoadDataReaction(QAction* parent);

protected:
  void onTriggered() override;

private:
  Q_DISABLE_COPY(pqSpreadsheetLoadDataReaction)
};

#endif
