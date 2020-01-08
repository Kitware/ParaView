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
/**
 * @class   vtkGridConnectivity
 * @brief   Integrates lines, surfaces and volume.
 *
 * Integrates all point and cell data attributes while computing
 * length, area or volume.  Works for 1D, 2D or 3D.  Only one dimensionality
 * at a time.  For volume, this filter ignores all but 3D cells.  It
 * will not compute the volume contained in a closed surface.
 * The output of this filter is a single point and vertex.  The attributes
 * for this point and cell will contain the integration results
 * for the corresponding input attributes.
*/

#ifndef vtkGridConnectivity_h
#define vtkGridConnectivity_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports
#include "vtkSmartPointer.h"                        // For ivars
#include <vector>                                   // For ivars

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

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkGridConnectivity
  : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkGridConnectivity, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkGridConnectivity* New();

  // Public so templated function can access this method.
  void IntegrateCellVolume(
    vtkCell* cell, int fragmentId, vtkUnstructuredGrid* input, vtkIdType cellIndex);

protected:
  vtkGridConnectivity();
  ~vtkGridConnectivity() override;

  vtkMultiProcessController* Controller;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  // I had to make this templated for global pointIds.
  // void ExecuteProcess(vtkUnstructuredGrid* inputs[],
  //                    int numberOfInputs);

  void GenerateOutput(vtkPolyData* output, vtkUnstructuredGrid* inputs[]);

  // Create a default executive.
  vtkExecutive* CreateDefaultExecutive() override;

  int FillInputPortInformation(int, vtkInformation*) override;

  // This method returns 1 if the input has the necessary arrays for this filter.
  int CheckInput(vtkUnstructuredGrid* grid);

  // Find the maximum global point id and allocate the hash.
  void InitializeFaceHash(vtkUnstructuredGrid** inputs, int numberOfInputs);
  vtkGridConnectivityFaceHash* FaceHash;

  void InitializeIntegrationArrays(vtkUnstructuredGrid** inputs, int numberOfInputs);

  vtkEquivalenceSet* EquivalenceSet;
  vtkDoubleArray* FragmentVolumes;

  std::vector<vtkSmartPointer<vtkDoubleArray> > CellAttributesIntegration;
  std::vector<vtkSmartPointer<vtkDoubleArray> > PointAttributesIntegration;

  // Temporary structures to help integration.
  vtkPoints* CellPoints;
  vtkIdList* CellPointIds;
  double IntegrateTetrahedron(vtkCell* tetra, vtkUnstructuredGrid* input, int fragmentId);
  double IntegrateHex(vtkCell* hex, vtkUnstructuredGrid* input, int fragmentId);
  double IntegrateVoxel(vtkCell* voxel, vtkUnstructuredGrid* input, int fragmentId);
  double IntegrateGeneral3DCell(vtkCell* cell, vtkUnstructuredGrid* input, int fragmentId);
  double ComputeTetrahedronVolume(double* pts0, double* pts1, double* pts2, double* pts3);
  void ComputePointIntegration(vtkUnstructuredGrid* input, vtkIdType pt0Id, vtkIdType pt1Id,
    vtkIdType pt2Id, vtkIdType pt3Id, double volume, int fragmentId);

  void ResolveIntegrationArrays();
  void ResolveFaceFragmentIds();

  short ProcessId;
  int GlobalPointIdType;

  void ResolveEquivalentFragments();
  void ResolveProcessesFaces();
  void CollectFacesAndArraysToRootProcess(int* fragmentIdMap, int* fragmentNumFaces);

private:
  vtkGridConnectivity(const vtkGridConnectivity&) = delete;
  void operator=(const vtkGridConnectivity&) = delete;
};

#endif
