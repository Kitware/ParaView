// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME vtkSamplePlaneSource - Create an oriented plane that can be used to sample the input.
//
// .SECTION Description
//
// This filter creates a plane with a given center point and normal.  This
// algorithm is designed to have an input whose bounds are used to establish the
// size (in world coordinates) of the plane.  The number of points in the x and
// y direction of the plane are generally set by the resolution, but the output
// will actually have more points than specified.  The plane geometry will be
// made larger than the bounds to ensure that, if used to probe the input, it
// will cover the entire geometry.
//

#ifndef vtkSamplePlaneSource_h
#define vtkSamplePlaneSource_h

#include "vtkPolyDataAlgorithm.h"
#include "vtkSLACFiltersModule.h" // for export macro

class vtkMultiProcessController;

class VTKSLACFILTERS_EXPORT vtkSamplePlaneSource : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkSamplePlaneSource, vtkPolyDataAlgorithm);
  static vtkSamplePlaneSource* New();
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

  // Description:
  // The controller used to determine the actual bounds of the input.  Use
  // a dummy controller if not running in parallel.
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  virtual void SetController(vtkMultiProcessController*);

protected:
  vtkSamplePlaneSource();
  ~vtkSamplePlaneSource() override;

  double Center[3];
  double Normal[3];
  int Resolution;

  vtkMultiProcessController* Controller;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  // Description:
  // Finds the bounds of a data object (can be either a vtkDataSet or
  // a vtkCompositeDataSet containing vtkDataSets).
  virtual void ComputeLocalBounds(vtkDataObject* input, double bounds[6]);

  // Description:
  // Resolves the bounds of all parallel processes.
  virtual void ResolveParallelBounds(double bounds[6]);

  // Description:
  // Creates the output poly data based on the bounds and ivars.
  virtual void CreatePlane(const double bounds[6], vtkPolyData* output);

private:
  vtkSamplePlaneSource(const vtkSamplePlaneSource&) = delete;
  void operator=(const vtkSamplePlaneSource&) = delete;
};

#endif // vtkSamplePlaneSource_h
