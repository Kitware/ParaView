/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataGeodesicDistance.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Copyright (c) 2013 Karthik Krishnan.
  Contributed to the VisualizationToolkit by the author under the terms
  of the Visualization Toolkit copyright

=========================================================================*/

// .NAME vtkPolyDataGeodesicDistance - Abstract base for classes that generate a geodesic path
// .SECTION Description
// Serves as a base class for algorithms that trace a geodesic path on a
// polygonal dataset.

#ifndef vtkPolyDataGeodesicDistance_h
#define vtkPolyDataGeodesicDistance_h

#include "vtkGeodesicMeasurementFiltersModule.h" // for export macro
#include "vtkPolyDataAlgorithm.h"

class vtkPolyData;
class vtkFloatArray;
class vtkIdList;

class VTKGEODESICMEASUREMENTFILTERS_EXPORT vtkPolyDataGeodesicDistance : public vtkPolyDataAlgorithm
{
public:
  // Description:
  // Standard methods for printing and determining type information.
  vtkTypeMacro(vtkPolyDataGeodesicDistance, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // A list of points (seeds) on the input mesh from which to perform fast
  // marching. These are pointIds from the input mesh. At least one seed must
  // be specified.
  virtual void SetSeeds(vtkIdList*);
  vtkGetObjectMacro(Seeds, vtkIdList);

  // Description:
  // Set/Get the name of the distance field data array that this class will
  // crate. This is a scalar floating precision field array representing the
  // geodesic distance from the vertex. If not set, this class will not
  // generate a distance field on the output. This may be useful for
  // interactive applications, which may set a termination criteria and want
  // just the visited point ids and their distances.
  vtkSetStringMacro(FieldDataName);
  vtkGetStringMacro(FieldDataName);

  // Overload GetMTime() because we depend on seeds
  vtkMTimeType GetMTime() override;

protected:
  vtkPolyDataGeodesicDistance();
  ~vtkPolyDataGeodesicDistance() override;

  // Get the distance field array on the polydata
  vtkFloatArray* GetGeodesicDistanceField(vtkPolyData* pd);

  // Compute the geodesic distance. Subclasses should override this method.
  // Returns 1 on success;
  virtual int Compute();

  char* FieldDataName;
  vtkIdList* Seeds;

private:
  vtkPolyDataGeodesicDistance(const vtkPolyDataGeodesicDistance&) = delete;
  void operator=(const vtkPolyDataGeodesicDistance&) = delete;
};

#endif
