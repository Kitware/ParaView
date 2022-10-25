/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDisplaySizedImplicitPlaneRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
