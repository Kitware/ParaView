// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME vtkSLACPlaneGlyphs - Create evenly spaced glyphs on a plane through data.
//
// .SECTION Description
//
// This filter probes a volume with regularly spaced samples on a plane and
// generates oriented glyphs.  It also supports some special scaling of
// the glyphs to look nice on electric field data.
//

#ifndef vtkSLACPlaneGlyphs_h
#define vtkSLACPlaneGlyphs_h

#include "vtkPolyDataAlgorithm.h"
#include "vtkSLACFiltersModule.h" // for export macro

class VTKSLACFILTERS_EXPORT vtkSLACPlaneGlyphs : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkSLACPlaneGlyphs, vtkPolyDataAlgorithm);
  static vtkSLACPlaneGlyphs* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // The location of the center of the plane.  A point is guaranteed to be here.
  vtkGetVector3Macro(Center, double);
  vtkSetVector3Macro(Center, double);

  // Description:
  // The normal to the plane.
  vtkGetVector3Macro(Normal, double);
  vtkSetVector3Macro(Normal, double);

  // Description:
  // The approximate number of samples in each direction that will intersect the
  // input bounds.
  vtkGetMacro(Resolution, int);
  vtkSetMacro(Resolution, int);

protected:
  vtkSLACPlaneGlyphs();
  ~vtkSLACPlaneGlyphs() override;

  double Center[3];
  double Normal[3];
  int Resolution;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkSLACPlaneGlyphs(const vtkSLACPlaneGlyphs&) = delete;
  void operator=(const vtkSLACPlaneGlyphs&) = delete;
};

#endif // vtkSLACPlaneGlyphs_h
