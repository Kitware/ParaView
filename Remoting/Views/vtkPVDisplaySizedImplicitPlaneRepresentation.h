// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVDisplaySizedImplicitPlaneRepresentation
 * @brief   extends vtkDisplaySizedImplicitPlaneRepresentation
 *
 * vtkPVDisplaySizedImplicitPlaneRepresentation extends vtkDisplaySizedImplicitPlaneRepresentation
 * to add ParaView proper initialisation values
 */

#ifndef vtkPVDisplaySizedImplicitPlaneRepresentation_h
#define vtkPVDisplaySizedImplicitPlaneRepresentation_h

#include "vtkDisplaySizedImplicitPlaneRepresentation.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class VTKREMOTINGVIEWS_EXPORT vtkPVDisplaySizedImplicitPlaneRepresentation
  : public vtkDisplaySizedImplicitPlaneRepresentation
{
public:
  static vtkPVDisplaySizedImplicitPlaneRepresentation* New();
  vtkTypeMacro(
    vtkPVDisplaySizedImplicitPlaneRepresentation, vtkDisplaySizedImplicitPlaneRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPVDisplaySizedImplicitPlaneRepresentation();
  ~vtkPVDisplaySizedImplicitPlaneRepresentation() override;

private:
  vtkPVDisplaySizedImplicitPlaneRepresentation(
    const vtkPVDisplaySizedImplicitPlaneRepresentation&) = delete;
  void operator=(const vtkPVDisplaySizedImplicitPlaneRepresentation&) = delete;
};

#endif
