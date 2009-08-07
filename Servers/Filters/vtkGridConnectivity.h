/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGridConnectivity.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGridConnectivity - Integrates lines, surfaces and volume.
// .SECTION Description
// Integrates all point and cell data attributes while computing
// length, area or volume.  Works for 1D, 2D or 3D.  Only one dimensionality
// at a time.  For volume, this filter ignores all but 3D cells.  It
// will not compute the volume contained in a closed surface.  
// The output of this filter is a single point and vertex.  The attributes
// for this point and cell will contain the integration results
// for the corresponding input attributes.

#ifndef __vtkGridConnectivity_h
#define __vtkGridConnectivity_h

#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkCell;
class vtkPoints;
class vtkDoubleArray;
class vtkIdList;
class vtkInformation;
class vtkInformationVector;
class vtkMultiProcessController;
class vtkGridConnectivityFaceHash;
class vtkEquivalenceSet;
class vtkUnstructuredGrid;
class vtkPolyData;

class VTK_EXPORT vtkGridConnectivity : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkGridConnectivity,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkGridConnectivity *New();
  
protected:
  vtkGridConnectivity();
  ~vtkGridConnectivity();

  vtkMultiProcessController* Controller;

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  void ExecuteProcess(vtkUnstructuredGrid* inputs[], 
                      int numberOfInputs);

  void GenerateOutput(vtkPolyData* output, vtkUnstructuredGrid* inputs[]);

  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive();

  virtual int FillInputPortInformation(int, vtkInformation*);

  // This method returns 1 if the input has the necessary arrays for this filter.
  int CheckInput(vtkUnstructuredGrid* grid);

  // Find the maximum global point id and allocate the hash.
  void InitializeFaceHash(vtkUnstructuredGrid** inputs, int numberOfInputs);
  vtkGridConnectivityFaceHash* FaceHash;

  vtkEquivalenceSet *EquivalenceSet;
  vtkDoubleArray* FragmentVolumes;
  void IntegrateCellVolume(vtkCell* cell, int fragmentId);
  // Temporary structures to help integration.
  vtkPoints* CellPoints;
  vtkIdList* CellPointIds;
  double IntegrateTetrahedron(vtkCell* tetra);
  double IntegrateHex(vtkCell* hex);
  double IntegrateVoxel(vtkCell* voxel);
  double IntegrateGeneral3DCell(vtkCell* cell);
  double ComputeTetrahedronVolume(
    double* pts0, double* pts1,
    double* pts2, double* pts3);
  void ResolveIntegrationArrays();
  void ResolveFaceFragmentIds();

  short ProcessId;

  void ResolveEquivalentFragments();
  void ResolveProcessesFaces();
  void CollectFacesAndArraysToRootProcess(int* fragmentIdOffsets);

private:
  vtkGridConnectivity(const vtkGridConnectivity&);  // Not implemented.
  void operator=(const vtkGridConnectivity&);  // Not implemented.
};

#endif
