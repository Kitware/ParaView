/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkIntersectFragments
 * @brief   Geometry intersection operations.
 *
 * TODO
*/

#ifndef vtkIntersectFragments_h
#define vtkIntersectFragments_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include <string>                            //
#include <vector>                            //

class vtkPolyData;
// class vtkMultiBlockDataSet;
class vtkPoints;
class vtkDoubleArray;
class vtkIntArray;
class vtkImplicitFunction;
class vtkMultiProcessController;
class vtkMaterialInterfaceCommBuffer;
class vtkCutter;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkIntersectFragments : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkIntersectFragments* New();
  vtkTypeMacro(vtkIntersectFragments, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// PARAVIEW interface stuff
  //@{
  /**
   * Specify the implicit function to perform the cutting.
   */
  virtual void SetCutFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(CutFunction, vtkImplicitFunction);
  //@}
  /**
   * Specify the geometry Input.
   */
  void SetGeometryInputConnection(vtkAlgorithmOutput* algOutput);
  /**
   * Specify the geometry Input.
   */
  void SetStatisticsInputConnection(vtkAlgorithmOutput* algOutput);
  /**
   * Override GetMTime because we refer to vtkImplicitFunction.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkIntersectFragments();
  ~vtkIntersectFragments() override;

  /// pipeline
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;
  ///
  // Make list of what we own
  int IdentifyLocalFragments();
  // Copy structure from multi block of polydata.
  int CopyInputStructureStats(vtkMultiBlockDataSet* dest, vtkMultiBlockDataSet* src);
  // Copy structure from multi block of multi piece
  int CopyInputStructureGeom(vtkMultiBlockDataSet* dest, vtkMultiBlockDataSet* src);
  //
  int PrepareToProcessRequest();
  //
  int Intersect();
  // Build arrays that describe which fragment
  // intersections are not empty.
  void BuildLoadingArray(std::vector<vtkIdType>& loadingArray, int blockId);
  int PackLoadingArray(vtkIdType*& buffer, int blockId);
  int UnPackLoadingArray(
    vtkIdType* buffer, int bufSize, std::vector<vtkIdType>& loadingArray, int blockId);
  //
  void ComputeGeometricAttributes();
  // Send my geometric attribuites to a single process.
  int SendGeometricAttributes(const int recipientProcId);
  // size buffers & new containers
  int PrepareToCollectGeometricAttributes(std::vector<vtkMaterialInterfaceCommBuffer>& buffers,
    std::vector<std::vector<vtkDoubleArray*> >& centers, std::vector<std::vector<int*> >& ids);
  // Free resources.
  int CleanUpAfterCollectGeometricAttributes(std::vector<vtkMaterialInterfaceCommBuffer>& buffers,
    std::vector<std::vector<vtkDoubleArray*> >& centers, std::vector<std::vector<int*> >& ids);
  // Receive all geometric attributes from all other
  // processes.
  int CollectGeometricAttributes(std::vector<vtkMaterialInterfaceCommBuffer>& buffers,
    std::vector<std::vector<vtkDoubleArray*> >& centers, std::vector<std::vector<int*> >& ids);
  // size local copy to hold all.
  int PrepareToMergeGeometricAttributes(std::vector<std::vector<int> >& unique);
  // Gather geometric attributes on a single process.
  int GatherGeometricAttributes(const int recipientProcId);
  // Copy attributes from input to output
  int CopyAttributesToStatsOutput(const int controllingProcId);
  //
  int CleanUpAfterRequest();

  /// data
  //
  vtkMultiProcessController* Controller;
  // Global ids of what we own before the intersection.
  std::vector<std::vector<int> > FragmentIds;
  // Centers, and global fragment ids.
  // an array for each block.
  std::vector<vtkDoubleArray*> IntersectionCenters;
  std::vector<std::vector<int> > IntersectionIds;
  //
  vtkCutter* Cutter;
  // data in/out
  vtkMultiBlockDataSet* GeomIn;
  vtkMultiBlockDataSet* GeomOut;
  vtkMultiBlockDataSet* StatsIn;
  vtkMultiBlockDataSet* StatsOut;
  int NBlocks;
  // only used on controller.
  std::vector<int> NFragmentsIntersected;

  /// PARAVIEW interface data
  vtkImplicitFunction* CutFunction;
  double Progress;
  double ProgressIncrement;

private:
  vtkIntersectFragments(const vtkIntersectFragments&) = delete;
  void operator=(const vtkIntersectFragments&) = delete;
};

#endif
