// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkPVImplicitConeRepresentation
 * @brief   extends vtkImplicitConeRepresentation
 *
 * vtkPVImplicitConeRepresentation extends vtkImplicitConeRepresentation
 * to add proper ParaView initialisation values
 */

#ifndef vtkPVImplicitConeRepresentation_h
#define vtkPVImplicitConeRepresentation_h

#include "vtkImplicitConeRepresentation.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class VTKREMOTINGVIEWS_EXPORT vtkPVImplicitConeRepresentation : public vtkImplicitConeRepresentation
{
public:
  static vtkPVImplicitConeRepresentation* New();
  vtkTypeMacro(vtkPVImplicitConeRepresentation, vtkImplicitConeRepresentation);

protected:
  vtkPVImplicitConeRepresentation();
  ~vtkPVImplicitConeRepresentation() override = default;

private:
  vtkPVImplicitConeRepresentation(const vtkPVImplicitConeRepresentation&) = delete;
  void operator=(const vtkPVImplicitConeRepresentation&) = delete;
};

#endif
