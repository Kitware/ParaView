/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUncertaintySurfaceRepresentation
// .SECTION Description
// vtkUncertaintySurfaceRepresentation extends vtkGeometryRepresentation
// render surfaces with both value and uncertainty data.

#ifndef vtkUncertaintySurfaceRepresentation_h
#define vtkUncertaintySurfaceRepresentation_h

#include "vtkGeometryRepresentation.h"

class vtkPiecewiseFunction;
class vtkUncertaintySurfacePainter;

class VTK_EXPORT vtkUncertaintySurfaceRepresentation : public vtkGeometryRepresentation
{
public:
  static vtkUncertaintySurfaceRepresentation* New();
  vtkTypeMacro(vtkUncertaintySurfaceRepresentation, vtkGeometryRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // Description:
  // Select the uncertainty array.
  void SelectUncertaintyArray(int, int, int, int, const char* name)
  {
    this->SetUncertaintyArray(name);
  }

  // Description:
  // Set/get the uncertainty array name.
  void SetUncertaintyArray(const char* name);
  const char* GetUncertaintyArray() const;

  // Description:
  // Set/get the uncertainty transfer function.
  void SetUncertaintyTransferFunction(vtkPiecewiseFunction* function);
  vtkPiecewiseFunction* GetUncertaintyTransferFunction() const;

  // Description:
  // Rescales the uncertainty transfer function to the data range.
  void RescaleUncertaintyTransferFunctionToDataRange();

  // Description:
  // Set/get the uncertainty scale factor.
  void SetUncertaintyScaleFactor(double density);
  double GetUncertaintyScaleFactor() const;

protected:
  vtkUncertaintySurfaceRepresentation();
  ~vtkUncertaintySurfaceRepresentation();

  void UpdateColoringParameters();

private:
  vtkUncertaintySurfacePainter* Painter;
};

#endif // vtkUncertaintySurfaceRepresentation_h
