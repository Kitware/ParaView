// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkPVImplicitFrustumRepresentation
 * @brief   extends vtkImplicitFrustumRepresentation
 *
 * vtkPVImplicitFrustumRepresentation extends vtkImplicitFrustumRepresentation
 * to set the default widget opacity to match other ParaView widgets.
 */

#ifndef vtkPVImplicitFrustumRepresentation_h
#define vtkPVImplicitFrustumRepresentation_h

#include "vtkImplicitFrustumRepresentation.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class VTKREMOTINGVIEWS_EXPORT vtkPVImplicitFrustumRepresentation
  : public vtkImplicitFrustumRepresentation
{
public:
  static vtkPVImplicitFrustumRepresentation* New();
  vtkTypeMacro(vtkPVImplicitFrustumRepresentation, vtkImplicitFrustumRepresentation);

private:
  vtkPVImplicitFrustumRepresentation();
  ~vtkPVImplicitFrustumRepresentation() override = default;

  vtkPVImplicitFrustumRepresentation(const vtkPVImplicitFrustumRepresentation&) = delete;
  void operator=(const vtkPVImplicitFrustumRepresentation&) = delete;
};

#endif
