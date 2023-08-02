// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStructuredGridVolumeRepresentation
 * @brief   representation for rendering vtkStructuredGrid as volume.
 *
 * vtkStructuredGridVolumeRepresentation is a representation for volume
 * rendering vtkStructuredGrid datasets. For the mast part the work is done by
 * vtkUnstructuredGridVolumeRepresentation. It converts the input
 * vtkStructuredGrid to vtkUnstructuredGrid and then renders it.
 */

#ifndef vtkStructuredGridVolumeRepresentation_h
#define vtkStructuredGridVolumeRepresentation_h

#include "vtkUnstructuredGridVolumeRepresentation.h"

class VTKREMOTINGVIEWS_EXPORT vtkStructuredGridVolumeRepresentation
  : public vtkUnstructuredGridVolumeRepresentation
{
public:
  static vtkStructuredGridVolumeRepresentation* New();
  vtkTypeMacro(vtkStructuredGridVolumeRepresentation, vtkUnstructuredGridVolumeRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkStructuredGridVolumeRepresentation();
  ~vtkStructuredGridVolumeRepresentation() override;

  /**
   * Fill input port information.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkStructuredGridVolumeRepresentation(const vtkStructuredGridVolumeRepresentation&) = delete;
  void operator=(const vtkStructuredGridVolumeRepresentation&) = delete;
};

#endif
