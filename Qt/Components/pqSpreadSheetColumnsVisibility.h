/*=========================================================================

Program: ParaView
Module:  pqSpreadSheetColumnsVisibility.h

Copyright (c) Kitware Inc.
All rights reserved.

See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

========================================================================*/
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
