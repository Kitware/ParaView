// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkPVColorTransferControlPointsItem_h
#define vtkPVColorTransferControlPointsItem_h

#include "pqComponentsModule.h"
#include "vtkColorTransferControlPointsItem.h"

/**
 * vtkPVColorTransferControlPointsItem overrides the VTK base class to customize
 * mouse/keyboard interaction.
 */
class PQCOMPONENTS_EXPORT vtkPVColorTransferControlPointsItem
  : public vtkColorTransferControlPointsItem
{
  vtkTypeMacro(vtkPVColorTransferControlPointsItem, vtkColorTransferControlPointsItem);

  static vtkPVColorTransferControlPointsItem* New();

  /// Ignore left double-click, in favor of Enter keypress event.
  bool MouseDoubleClickEvent(const vtkContextMouseEvent& mouse) override;
  /// Use Enter/Return to start a point-edit event
  bool KeyPressEvent(const vtkContextKeyEvent& key) override;

protected:
  vtkPVColorTransferControlPointsItem();
  ~vtkPVColorTransferControlPointsItem() override;

private:
  vtkPVColorTransferControlPointsItem(const vtkPVColorTransferControlPointsItem&) = delete;
  void operator=(const vtkPVColorTransferControlPointsItem&) = delete;
};

#endif
