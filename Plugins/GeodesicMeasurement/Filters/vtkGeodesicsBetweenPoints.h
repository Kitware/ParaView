/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFastMarchingGeodesicPath.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkGeodesicBetweenPoints - Computes geodesics on input vtkPolyData between pairs of
// consecutive points
// .SECTION Description
// The class computes geodesic paths on vtkPolyData between pairs of consecutive points.
// The vtkPolyData is the first input and the vtkPointSet input is the second input.
// For each point in the second input, the nearest point on the first input is found.
// Geodesic paths between consective points in the second input are then computed
// and returned in the output. In addition, the total length of the geodesic paths
// is computed and stored in a one-element array named "TotalLength" in the field data
// of the output.
#ifndef vtkGeodesicsBetweenPoints_h
#define vtkGeodesicsBetweenPoints_h

#include "vtkGeodesicMeasurementFiltersModule.h" // for export macro
#include "vtkPolyDataAlgorithm.h"

class VTKGEODESICMEASUREMENTFILTERS_EXPORT vtkGeodesicsBetweenPoints : public vtkPolyDataAlgorithm
{
public:
  static vtkGeodesicsBetweenPoints* New();

  // Description:
  // Standard methods for printing and determining type information.
  vtkTypeMacro(vtkGeodesicsBetweenPoints, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Set the dataset that defines the endpoints between which geodesic
  // paths should be computed. Geodesics will be computed between
  // consective points in this input.
  virtual void SetEndpointsConnection(vtkAlgorithmOutput* endpointConnection);

  // Description:
  // Compute a loop of geodesic paths. Off by default. If on, a
  // geodesic path from the last to the first point in the input
  // endpoints will be added to the output.
  vtkSetMacro(Loop, int);
  vtkGetMacro(Loop, int);
  vtkBooleanMacro(Loop, int);

  // Description:
  // Connect the first and last points with a straight line
  // segment. Off by default.
  vtkSetMacro(LoopWithLine, int);
  vtkGetMacro(LoopWithLine, int);
  vtkBooleanMacro(LoopWithLine, int);

protected:
  vtkGeodesicsBetweenPoints();
  ~vtkGeodesicsBetweenPoints() override;

  int Loop;
  int LoopWithLine;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkGeodesicsBetweenPoints(const vtkGeodesicsBetweenPoints&) = delete;
  void operator=(const vtkGeodesicsBetweenPoints&) = delete;
};

#endif
