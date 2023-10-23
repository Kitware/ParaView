// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVClipClosedSurface
 * @brief   Clipper for generating closed surfaces
 *
 *
 *  This is a subclass of vtkClipClosedSurface
 */

#ifndef vtkPVClipClosedSurface_h
#define vtkPVClipClosedSurface_h

#include "vtkClipClosedSurface.h"
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports

class vtkPlane;

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPVClipClosedSurface : public vtkClipClosedSurface
{
public:
  vtkTypeMacro(vtkPVClipClosedSurface, vtkClipClosedSurface);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPVClipClosedSurface* New();

  /**
   * Set the clipping plane.
   */
  void SetClippingPlane(vtkPlane* plane);

protected:
  vtkPVClipClosedSurface();
  ~vtkPVClipClosedSurface() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  vtkPlane* ClippingPlane;

private:
  vtkPVClipClosedSurface(const vtkPVClipClosedSurface&) = delete;
  void operator=(const vtkPVClipClosedSurface&) = delete;
};

#endif
