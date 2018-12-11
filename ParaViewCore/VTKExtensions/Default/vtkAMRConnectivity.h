/*=========================================================================

  Program:   ParaView
  Module:    vtkAMRConnectivity.h

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  Copyright 2013 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.

=========================================================================*/
/**
 * @class   vtkAMRConnectivity
 * @brief   Identify fragments in the grid
 *
 *
 *
 * .SEE vtkAMRConnectivity
*/

#ifndef vtkAMRConnectivity_h
#define vtkAMRConnectivity_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkSmartPointer.h"                 // needed for vtkSmartPointer.
#include <string>                            // STL required.
#include <vector>                            // STL required.

class vtkNonOverlappingAMR;
class vtkUniformGrid;
class vtkIdTypeArray;
class vtkIntArray;
class vtkAMRDualGridHelper;
class vtkAMRDualGridHelperBlock;
class vtkAMRConnectivityEquivalence;
class vtkMPIController;
class vtkUnsignedCharArray;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkAMRConnectivity : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkAMRConnectivity, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkAMRConnectivity* New();

  //@{
  /**
   * Add to list of volume arrays to find connected fragments
   */
  void AddInputVolumeArrayToProcess(const char* name);
  void ClearInputVolumeArrayToProcess();
  //@}

  //@{
  /**
   * Get / Set volume fraction value.
   */
  vtkGetMacro(VolumeFractionSurfaceValue, double);
  vtkSetMacro(VolumeFractionSurfaceValue, double);
  //@}

  //@{
  /**
   * Get / Set where to resolve the regions between blocks
   */
  vtkGetMacro(ResolveBlocks, bool);
  vtkSetMacro(ResolveBlocks, bool);
  //@}

  //@{
  /**
   * Get / Set where to resolve the regions between blocks
   */
  vtkGetMacro(PropagateGhosts, bool);
  vtkSetMacro(PropagateGhosts, bool);
  //@}

protected:
  vtkAMRConnectivity();
  ~vtkAMRConnectivity() override;

  double VolumeFractionSurfaceValue;
  vtkAMRDualGridHelper* Helper;
  vtkAMRConnectivityEquivalence* Equivalence;

  bool ResolveBlocks;
  bool PropagateGhosts;

  std::string RegionName;
  vtkIdType NextRegionId;

  std::vector<std::string> VolumeArrays;

  std::vector<std::vector<vtkSmartPointer<vtkIdTypeArray> > > BoundaryArrays;
  std::vector<std::vector<int> > ReceiveList;

  std::vector<bool> ValidNeighbor;
  std::vector<std::vector<std::vector<int> > > NeighborList;
  std::vector<vtkSmartPointer<vtkIntArray> > EquivPairs;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int DoRequestData(vtkNonOverlappingAMR*, const char*);
  int WavePropagation(vtkIdType cellIdStart, vtkUniformGrid* grid, vtkIdTypeArray* regionId,
    vtkDataArray* volArray, vtkUnsignedCharArray* ghostArray);

  vtkAMRDualGridHelperBlock* GetBlockNeighbor(vtkAMRDualGridHelperBlock* block, int dir);
  void ProcessBoundaryAtBlock(vtkNonOverlappingAMR* volume, vtkAMRDualGridHelperBlock* block,
    vtkAMRDualGridHelperBlock* neighbor, int dir);
  int ExchangeBoundaries(vtkMPIController* controller);
  int ExchangeEquivPairs(vtkMPIController* controller);
  void ProcessBoundaryAtNeighbor(vtkNonOverlappingAMR* volume, vtkIdTypeArray* array);

private:
  vtkAMRConnectivity(const vtkAMRConnectivity&) = delete;
  void operator=(const vtkAMRConnectivity&) = delete;
};

#endif /* vtkAMRConnectivity_h */
