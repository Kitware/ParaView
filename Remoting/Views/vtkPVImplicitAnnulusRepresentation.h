// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkPVImplicitAnnulusRepresentation
 * @brief   extends vtkImplicitAnnulusRepresentation
 *
 * vtkPVImplicitAnnulusRepresentation extends vtkImplicitAnnulusRepresentation
 * to add proper ParaView initialisation values
 */

#ifndef vtkPVImplicitAnnulusRepresentation_h
#define vtkPVImplicitAnnulusRepresentation_h

#include "vtkImplicitAnnulusRepresentation.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class VTKREMOTINGVIEWS_EXPORT vtkPVImplicitAnnulusRepresentation
  : public vtkImplicitAnnulusRepresentation
{
public:
  static vtkPVImplicitAnnulusRepresentation* New();
  vtkTypeMacro(vtkPVImplicitAnnulusRepresentation, vtkImplicitAnnulusRepresentation);

private:
  vtkPVImplicitAnnulusRepresentation();
  ~vtkPVImplicitAnnulusRepresentation() override = default;

  vtkPVImplicitAnnulusRepresentation(const vtkPVImplicitAnnulusRepresentation&) = delete;
  void operator=(const vtkPVImplicitAnnulusRepresentation&) = delete;
};

#endif
