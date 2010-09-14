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
// .NAME vtkIntersectFragments - Geometry intersection operations.
// .SECTION Description
// TODO

#ifndef __vtkIntersectFragments_h
#define __vtkIntersectFragments_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkstd/vector"//
#include "vtkstd/string"//

class vtkPolyData;
//class vtkMultiBlockDataSet;
class vtkPoints;
class vtkDoubleArray;
class vtkIntArray;
class vtkImplicitFunction;
class vtkMultiProcessController;
class vtkMaterialInterfaceCommBuffer;
class vtkCutter;

class VTK_EXPORT vtkIntersectFragments : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkIntersectFragments *New();
  vtkTypeMacro(vtkIntersectFragments,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// PARAVIEW interface stuff
  // Description
  // Specify the implicit function to perform the cutting.
  virtual void SetCutFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(CutFunction,vtkImplicitFunction);
  // Description:
  // Specify the geometry Input.
  void SetGeometryInputConnection(vtkAlgorithmOutput* algOutput);
  // Description:
  // Specify the geometry Input.
  void SetStatisticsInputConnection(vtkAlgorithmOutput* algOutput);
  // Description:
  // Override GetMTime because we refer to vtkImplicitFunction.
  unsigned long GetMTime();

protected:
  vtkIntersectFragments();
  ~vtkIntersectFragments();

  //BTX
  /// pipeline
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);
  virtual int FillOutputPortInformation(int port, vtkInformation *info);
  ///
  // Make list of what we own
  int IdentifyLocalFragments();
  // Copy structure from multi block of polydata.
  int CopyInputStructureStats(
        vtkMultiBlockDataSet *dest,
        vtkMultiBlockDataSet *src);
  // Copy structure from mutli block of multi piece
  int CopyInputStructureGeom(
        vtkMultiBlockDataSet *dest,
        vtkMultiBlockDataSet *src);
  //
  int PrepareToProcessRequest();
  //
  int Intersect();
  // Build arrays that describe which fragment
  // intersections are not empty.
  void BuildLoadingArray(
          vtkstd::vector<vtkIdType> &loadingArray,
          int blockId);
  int PackLoadingArray(vtkIdType *&buffer, int blockId);
  int UnPackLoadingArray(
          vtkIdType *buffer,
          int bufSize,
          vtkstd::vector<vtkIdType> &loadingArray,
          int blockId);
  //
  void ComputeGeometricAttributes();
  // Send my geometric attribuites to a single process.
  int SendGeometricAttributes(const int recipientProcId);
  // size buffers & new containers
  int PrepareToCollectGeometricAttributes(
          vtkstd::vector<vtkMaterialInterfaceCommBuffer> &buffers,
          vtkstd::vector<vtkstd::vector<vtkDoubleArray *> >&centers,
          vtkstd::vector<vtkstd::vector<int *> >&ids);
  // Free resources.
  int CleanUpAfterCollectGeometricAttributes(
          vtkstd::vector<vtkMaterialInterfaceCommBuffer> &buffers,
          vtkstd::vector<vtkstd::vector<vtkDoubleArray *> >&centers,
          vtkstd::vector<vtkstd::vector<int *> >&ids);
  // Recieve all geometric attributes from all other
  // processes.
  int CollectGeometricAttributes(
          vtkstd::vector<vtkMaterialInterfaceCommBuffer> &buffers,
          vtkstd::vector<vtkstd::vector<vtkDoubleArray *> >&centers,
          vtkstd::vector<vtkstd::vector<int *> >&ids);
  // size local copy to hold all.
  int PrepareToMergeGeometricAttributes(
          vtkstd::vector<vtkstd::vector<int> >&unique);
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
  vtkstd::vector<vtkstd::vector<int> >FragmentIds;
  // Centers, and global fragment ids.
  // an array for each block.
  vtkstd::vector<vtkDoubleArray *>IntersectionCenters;
  vtkstd::vector<vtkstd::vector<int> >IntersectionIds;
  //
  vtkCutter *Cutter;
  // data in/out
  vtkMultiBlockDataSet *GeomIn;
  vtkMultiBlockDataSet *GeomOut;
  vtkMultiBlockDataSet *StatsIn;
  vtkMultiBlockDataSet *StatsOut;
  int NBlocks;
  // only used on controller.
  vtkstd::vector<int> NFragmentsIntersected;

  /// PARAVIEW interface data
  vtkImplicitFunction *CutFunction;
  double Progress;
  double ProgressIncrement;

private:
  vtkIntersectFragments(const vtkIntersectFragments&);  // Not implemented.
  void operator=(const vtkIntersectFragments&);  // Not implemented.
  //ETX
};

#endif
