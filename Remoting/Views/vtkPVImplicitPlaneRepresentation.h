// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVImplicitPlaneRepresentation
 * @brief   extends vtkImplicitPlaneRepresentation
 *
 * vtkPVImplicitPlaneRepresentation extends vtkImplicitPlaneRepresentation to
 * add ParaView proper initialisation values
 */

#ifndef vtkPVImplicitPlaneRepresentation_h
#define vtkPVImplicitPlaneRepresentation_h

#include "vtkImplicitPlaneRepresentation.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class VTKREMOTINGVIEWS_EXPORT vtkPVImplicitPlaneRepresentation
  : public vtkImplicitPlaneRepresentation
{
public:
  static vtkPVImplicitPlaneRepresentation* New();
  vtkTypeMacro(vtkPVImplicitPlaneRepresentation, vtkImplicitPlaneRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPVImplicitPlaneRepresentation();
  ~vtkPVImplicitPlaneRepresentation() override;

private:
  vtkPVImplicitPlaneRepresentation(const vtkPVImplicitPlaneRepresentation&) = delete;
  void operator=(const vtkPVImplicitPlaneRepresentation&) = delete;
};

#endif
