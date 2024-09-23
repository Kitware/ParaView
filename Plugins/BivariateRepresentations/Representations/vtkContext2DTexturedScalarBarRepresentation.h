// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkContext2DTexturedScalarBarRepresentation
 * @brief   Represent scalar bar for vtkScalarBarWidget.
 *
 * Subclass of vtkScalarBarRepresentation that considers viewport size
 * for placement of the representation. Similar to vtkPVScalarBarRepresentation
 * but renders a vtkContext2DTexturedScalarBarActor instead.
 *
 * @sa vtkPVScalarBarRepresentation vtkContext2DScalarBarActor vtkContext2DTexturedScalarBarActor
 */

#ifndef vtkContext2DTexturedScalarBarRepresentation_h
#define vtkContext2DTexturedScalarBarRepresentation_h

#include "vtkBivariateRepresentationsModule.h" // for export macro

#include "vtkScalarBarRepresentation.h"

class VTKBIVARIATEREPRESENTATIONS_EXPORT vtkContext2DTexturedScalarBarRepresentation
  : public vtkScalarBarRepresentation
{
public:
  vtkTypeMacro(vtkContext2DTexturedScalarBarRepresentation, vtkScalarBarRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkContext2DTexturedScalarBarRepresentation* New();

  /**
   * Override to obtain viewport size and potentially adjust placement
   * of the representation.
   */
  int RenderOverlay(vtkViewport*) override;

protected:
  vtkContext2DTexturedScalarBarRepresentation() = default;
  ~vtkContext2DTexturedScalarBarRepresentation() override = default;

private:
  vtkContext2DTexturedScalarBarRepresentation(
    const vtkContext2DTexturedScalarBarRepresentation&) = delete;
  void operator=(const vtkContext2DTexturedScalarBarRepresentation&) = delete;
};

#endif // vtkContext2DTexturedScalarBarRepresentation_h
