/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMantaTestSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMantaTestSource - produce triangles to benchmark manta with
// .SECTION Description
// This class produces polydata with a configurable number of triangles.
// It respects parallelism, so that parallel scaling can be tested, and the
// produced data is invariant with respect to the number of processors.
// Configuration options change the charactersitics of the generated "scene"

#ifndef vtkMantaTestSource_h
#define vtkMantaTestSource_h

#include "vtkMantaModule.h"
#include "vtkPolyDataAlgorithm.h"

class VTKMANTA_EXPORT vtkMantaTestSource : public vtkPolyDataAlgorithm
{
public:
  // Description:
  // Create a new instance with (50,50,50) points in the (u-v-w) directions.
  static vtkMantaTestSource* New();
  vtkTypeMacro(vtkMantaTestSource, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the number of triangles to produce.
  // Default is 100 triangles
  vtkSetMacro(Resolution, vtkIdType);
  vtkGetMacro(Resolution, vtkIdType);

  // Description:
  // Set/Get the spatial locality
  // 0.0 behind infinite, 1.0 being none,
  // this affects distance beteen sequential points
  // Default is 0.1
  vtkSetMacro(DriftFactor, double);
  vtkGetMacro(DriftFactor, double);

  // Description:
  // Set/Get the memory locality
  // 0.0 behind infinite (tri uses sequential pts),
  // 1.0 being none (tri uses any point)
  // Default is 0.01
  vtkSetMacro(SlidingWindow, double);
  vtkGetMacro(SlidingWindow, double);

protected:
  vtkMantaTestSource();
  ~vtkMantaTestSource();

  int RequestInformation(
    vtkInformation* info, vtkInformationVector** input, vtkInformationVector* output);

  int RequestData(vtkInformation* info, vtkInformationVector** input, vtkInformationVector* output);

  vtkIdType Resolution;
  double DriftFactor;
  double SlidingWindow;

private:
  vtkMantaTestSource(const vtkMantaTestSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMantaTestSource&) VTK_DELETE_FUNCTION;
};

#endif
