/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIntegrateAttributes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIntegrateAttributes - Integrates lines, surfaces and volume.
// .SECTION Description
// Integrates all point and cell data attributes while computing
// length, area or volume.  Works for 1D, 2D or 3D.  Only one dimensionality
// at a time.  For volume, this filter ignores all but 3D cells.  It
// will not compute the volume contained in a closed surface.  
// The output of this filter is a single point and vertex.  The attributes
// for this point and cell will contain the integration results
// for the corresponding input attributes.

#ifndef __vtkIntegrateAttributes_h
#define __vtkIntegrateAttributes_h

#include "vtkDataSetToUnstructuredGridFilter.h"

class vtkDataSet;
class vtkIdList;
class vtkDataSetAttributes;
class vtkMultiProcessController;

class VTK_EXPORT vtkIntegrateAttributes : public vtkDataSetToUnstructuredGridFilter
{
public:
  vtkTypeRevisionMacro(vtkIntegrateAttributes,vtkDataSetToUnstructuredGridFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkIntegrateAttributes *New();
  
protected:
  vtkIntegrateAttributes();
  ~vtkIntegrateAttributes();

  vtkMultiProcessController* Controller;

  // Usual data generation method
  void Execute();

  int CompareIntegrationDimension(int dim);
  int IntegrationDimension;

  // The length, area or volume of the data set.  Computed by Execute;
  double Sum;
  // ToCompute the location of the output point.
  double SumCenter[3];

  void IntegratePolyLine(vtkDataSet* input, 
                         vtkUnstructuredGrid* output,
                         vtkIdType cellId, vtkIdList* cellPtIds);
  void IntegratePolygon(vtkDataSet* input, 
                         vtkUnstructuredGrid* output,
                         vtkIdType cellId, vtkIdList* cellPtIds);
  void IntegrateTriangleStrip(vtkDataSet* input, 
                         vtkUnstructuredGrid* output,
                         vtkIdType cellId, vtkIdList* cellPtIds);
  void IntegrateTriangle(vtkDataSet* input, 
                         vtkUnstructuredGrid* output,
                         vtkIdType cellId, vtkIdType pt1Id,
                         vtkIdType pt2Id, vtkIdType pt3Id);
  void IntegrateSatelliteData(vtkDataSetAttributes* inda,
                              vtkDataSetAttributes* outda);                  
  void AllocateAttributes(vtkDataSetAttributes* inda, 
                          vtkDataSetAttributes* outda);
  void ZeroAttributes(vtkDataSetAttributes* outda);
  void IntegrateData1(vtkDataSetAttributes* inda,
                      vtkDataSetAttributes* outda,
                      vtkIdType pt1Id, double k);
  void IntegrateData2(vtkDataSetAttributes* inda,
                      vtkDataSetAttributes* outda,
                      vtkIdType pt1Id, vtkIdType pt2Id, double k);
  void IntegrateData3(vtkDataSetAttributes* inda,
                      vtkDataSetAttributes* outda, vtkIdType pt1Id, 
                      vtkIdType pt2Id, vtkIdType pt3Id, double k);

private:
  vtkIntegrateAttributes(const vtkIntegrateAttributes&);  // Not implemented.
  void operator=(const vtkIntegrateAttributes&);  // Not implemented.
};

#endif
