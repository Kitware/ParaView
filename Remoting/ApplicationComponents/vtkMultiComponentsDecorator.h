// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkMultiComponentsDecorator_h
#define vtkMultiComponentsDecorator_h

#include "vtkPropertyDecorator.h"
#include "vtkRemotingApplicationComponentsModule.h"

/**
 * vtkMultiComponentsDecorator's purpose is to prevent the GUI from
 * showing Multi Components Mapping checkbox when the representation is not Volume,
 * the number of components is not valid or MapScalars is not checked.
 */
class VTKREMOTINGAPPLICATIONCOMPONENTS_EXPORT vtkMultiComponentsDecorator
  : public vtkPropertyDecorator
{
public:
  static vtkMultiComponentsDecorator* New();
  vtkTypeMacro(vtkMultiComponentsDecorator, vtkPropertyDecorator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Initialize(vtkPVXMLElement* config, vtkSMProxy* proxy) override;
  /**
   * Overridden to hide the widget
   */
  bool CanShow(bool show_advanced) const override;

protected:
  vtkMultiComponentsDecorator();
  ~vtkMultiComponentsDecorator() override;

private:
  vtkMultiComponentsDecorator(const vtkMultiComponentsDecorator&) = delete;
  void operator=(const vtkMultiComponentsDecorator&) = delete;

  std::vector<int> Components;
};

#endif
