// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSpreadSheetColumnsVisibility_h
#define pqSpreadSheetColumnsVisibility_h

#include "pqComponentsModule.h"

#include "pqSpreadSheetViewModel.h"
#include "vtkSMViewProxy.h"

#include <QCheckBox>
#include <QMenu>
#include <QWidgetAction>

/**
 * Generate a menu to control what columns we want to display
 * from a pqSpreadSheetViewModel.
 */
class PQCOMPONENTS_EXPORT pqSpreadSheetColumnsVisibility
{
public:
  /**
   * Fill a menu with checkbox for all columns in pqSpreadSheetViewModel.
   */
  static void populateMenu(vtkSMViewProxy* proxy, pqSpreadSheetViewModel* model, QMenu* menu);

private:
  static QCheckBox* addCheckableAction(QMenu* menu, const QString& text, const bool checked);

  static void updateAllCheckState(QCheckBox* allCheckbox, const std::vector<QCheckBox*>& cboxes);
};

#endif
