// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkANARIHidingDecorator_h
#define vtkANARIHidingDecorator_h

#include "vtkPropertyDecorator.h"

/**
 * vtkANARIHidingDecorator's purpose is to prevent the GUI from
 * showing any of the ANARI specific rendering controls when
 * Paraview is not configured with PARAVIEW_ENABLE_ANARI
 */
class VTKREMOTINGAPPLICATIONCOMPONENTS_EXPORT vtkANARIHidingDecorator : public vtkPropertyDecorator
{
public:
  static vtkANARIHidingDecorator* New();
  vtkTypeMacro(vtkANARIHidingDecorator, vtkPropertyDecorator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to hide the widget when ANARI is not compiled in
   */
  bool CanShow(bool showAdvanced) const override;

protected:
  vtkANARIHidingDecorator() = default;
  ~vtkANARIHidingDecorator() override = default;

  vtkANARIHidingDecorator(const vtkANARIHidingDecorator&) = delete;

private:
  void operator=(const vtkANARIHidingDecorator&) = delete;
};
#endif
