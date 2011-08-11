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

#include "vtkUnstructuredGridAlgorithm.h"

class vtkDataSet;
class vtkIdList;
class vtkInformation;
class vtkInformationVector;
class vtkDataSetAttributes;
class vtkMultiProcessController;

class VTK_EXPORT vtkIntegrateAttributes : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkIntegrateAttributes,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkIntegrateAttributes *New();

  void SetController(vtkMultiProcessController *controller);

//BTX
protected:
  vtkIntegrateAttributes();
  ~vtkIntegrateAttributes();

  vtkMultiProcessController* Controller;

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive();

  virtual int FillInputPortInformation(int, vtkInformation*);


  int CompareIntegrationDimension(vtkDataSet* output, int dim);
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
  void IntegrateTetrahedron(vtkDataSet* input,
                            vtkUnstructuredGrid* output,
                            vtkIdType cellId, vtkIdType pt1Id,
                            vtkIdType pt2Id, vtkIdType pt3Id,
                            vtkIdType pt4Id);
  void IntegratePixel(vtkDataSet* input,
                      vtkUnstructuredGrid* output,
                      vtkIdType cellId, vtkIdList* cellPtIds);
  void IntegrateVoxel(vtkDataSet* input,
                      vtkUnstructuredGrid* output,
                      vtkIdType cellId, vtkIdList* cellPtIds);
  void IntegrateGeneral1DCell(vtkDataSet* input,
                              vtkUnstructuredGrid* output,
                              vtkIdType cellId,
                              vtkIdList* cellPtIds);
  void IntegrateGeneral2DCell(vtkDataSet* input,
                              vtkUnstructuredGrid* output,
                              vtkIdType cellId,
                              vtkIdList* cellPtIds);
  void IntegrateGeneral3DCell(vtkDataSet* input,
                              vtkUnstructuredGrid* output,
                              vtkIdType cellId,
                              vtkIdList* cellPtIds);
  void IntegrateSatelliteData(vtkDataSetAttributes* inda,
                              vtkDataSetAttributes* outda);
  void ZeroAttributes(vtkDataSetAttributes* outda);

private:
  vtkIntegrateAttributes(const vtkIntegrateAttributes&);  // Not implemented.
  void operator=(const vtkIntegrateAttributes&);  // Not implemented.

  class vtkFieldList;
  vtkFieldList* CellFieldList;
  vtkFieldList* PointFieldList;
  int FieldListIndex;

  void AllocateAttributes(
    vtkFieldList& fieldList, vtkDataSetAttributes* outda);
  void ExecuteBlock(vtkDataSet* input, vtkUnstructuredGrid* output,
    int fieldset_index, vtkFieldList& pdList, vtkFieldList& cdList);

  void IntegrateData1(vtkDataSetAttributes* inda,
                      vtkDataSetAttributes* outda,
                      vtkIdType pt1Id, double k,
                      vtkFieldList& fieldlist,
                      int fieldlist_index);
  void IntegrateData2(vtkDataSetAttributes* inda,
                      vtkDataSetAttributes* outda,
                      vtkIdType pt1Id, vtkIdType pt2Id, double k,
                      vtkFieldList& fieldlist,
                      int fieldlist_index);
  void IntegrateData3(vtkDataSetAttributes* inda,
                      vtkDataSetAttributes* outda, vtkIdType pt1Id,
                      vtkIdType pt2Id, vtkIdType pt3Id, double k,
                      vtkFieldList& fieldlist,
                      int fieldlist_index);
  void IntegrateData4(vtkDataSetAttributes* inda,
                      vtkDataSetAttributes* outda, vtkIdType pt1Id,
                      vtkIdType pt2Id, vtkIdType pt3Id, vtkIdType pt4Id,
                      double k,
                      vtkFieldList& fieldlist,
                      int fieldlist_index);
public:
  enum CommunicationIds
   {
     IntegrateAttrInfo=2000,
     IntegrateAttrData
   };
//ETX
};

#endif
