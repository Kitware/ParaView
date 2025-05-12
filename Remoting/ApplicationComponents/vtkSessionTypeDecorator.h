// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkSessionTypeDecorator_h
#define vtkSessionTypeDecorator_h
#include "vtkPropertyDecorator.h"
#include "vtkRemotingApplicationComponentsModule.h"

/**
 * vtkSessionTypeDecorator is a vtkPropertyDecorator subclass that can be
 * used to control show/hide or enable/disable state based on the current
 * session.
 *
 * The XML config for this decorate takes two attributes 'requires' and 'mode'.
 * 'mode' can have values 'visibility' or 'enabled_state' which dictates whether
 * the decorator shows/hides or enables/disables the widget respectively.
 *
 * 'requires' can have values 'remote', 'parallel', 'parallel_data_server', or
 * 'parallel_render_server' indicating if the session must be remote, or parallel
 * with either data server or render server having more than 1 rank, or parallel
 * data-server, or parallel render-server respectively.
 */
class VTKREMOTINGAPPLICATIONCOMPONENTS_EXPORT vtkSessionTypeDecorator : public vtkPropertyDecorator
{

public:
  static vtkSessionTypeDecorator* New();
  vtkTypeMacro(vtkSessionTypeDecorator, vtkPropertyDecorator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Initialize(vtkPVXMLElement* config, vtkSMProxy* proxy) override;

  /**
   * Overridden to enable/disable the widget based on input data type.
   */
  bool Enable() const override;

  /**
   * Overriden to show or not the widget based on input data type.
   */
  bool CanShow(bool show_advanced) const override;

protected:
  vtkSessionTypeDecorator();
  ~vtkSessionTypeDecorator() override;

private:
  vtkSessionTypeDecorator(const vtkSessionTypeDecorator&) = delete;
  void operator=(const vtkSessionTypeDecorator&) = delete;

  bool IsVisible{ true };
  bool IsEnabled{ true };
};

#endif
