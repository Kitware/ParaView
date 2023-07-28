// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVImplicitCylinderRepresentation
 * @brief   extends vtkImplicitCylinderRepresentation
 *
 * vtkPVImplicitCylinderRepresentation extends vtkImplicitCylinderRepresentation
 * to add ParaView proper initialisation values
 */

#ifndef vtkPVImplicitCylinderRepresentation_h
#define vtkPVImplicitCylinderRepresentation_h

#include "vtkImplicitCylinderRepresentation.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class VTKREMOTINGVIEWS_EXPORT vtkPVImplicitCylinderRepresentation
  : public vtkImplicitCylinderRepresentation
{
public:
  static vtkPVImplicitCylinderRepresentation* New();
  vtkTypeMacro(vtkPVImplicitCylinderRepresentation, vtkImplicitCylinderRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPVImplicitCylinderRepresentation();
  ~vtkPVImplicitCylinderRepresentation() override;

private:
  vtkPVImplicitCylinderRepresentation(const vtkPVImplicitCylinderRepresentation&) = delete;
  void operator=(const vtkPVImplicitCylinderRepresentation&) = delete;
};

#endif
