// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkPVScalarBarRepresentation
 * @brief   Represent scalar bar for vtkScalarBarWidget.
 *
 * Subclass of vtkScalarBarRepresentation that considers viewport size
 * for placement of the representation.
 */

#ifndef vtkPVScalarBarRepresentation_h
#define vtkPVScalarBarRepresentation_h

#include "vtkRemotingViewsModule.h" // needed for export macro

#include "vtkScalarBarRepresentation.h"

class VTKREMOTINGVIEWS_EXPORT vtkPVScalarBarRepresentation : public vtkScalarBarRepresentation
{
public:
  vtkTypeMacro(vtkPVScalarBarRepresentation, vtkScalarBarRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPVScalarBarRepresentation* New();

  /**
   * Override to obtain viewport size and potentially adjust placement
   * of the representation.
   */
  int RenderOverlay(vtkViewport*) override;

protected:
  vtkPVScalarBarRepresentation() = default;
  ~vtkPVScalarBarRepresentation() override = default;

private:
  vtkPVScalarBarRepresentation(const vtkPVScalarBarRepresentation&) = delete;
  void operator=(const vtkPVScalarBarRepresentation&) = delete;
};

#endif // vtkPVScalarBarRepresentation_h
