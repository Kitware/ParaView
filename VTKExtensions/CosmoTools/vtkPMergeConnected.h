/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPMergeConnected.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   vtkPMergeConnected
 *
 *
 *  This filter merges connected voroni tessellation regions based on the
 *  global region ID.
*/

#ifndef vtkPMergeConnected_h
#define vtkPMergeConnected_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkPVVTKExtensionsCosmoToolsModule.h" // For export macro

class vtkMultiProcessController;
class vtkUnstructuredGrid;
class vtkIdList;
class vtkFloatArray;
class vtkIdTypeArray;

class VTKPVVTKEXTENSIONSCOSMOTOOLS_EXPORT vtkPMergeConnected : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkPMergeConnected* New();
  vtkTypeMacro(vtkPMergeConnected, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  struct FaceWithKey
  {
    int num_pts;
    vtkIdType *key, *orig;
  };
  struct cmp_ids;

protected:
  vtkPMergeConnected();
  ~vtkPMergeConnected();

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  int FillOutputPortInformation(int port, vtkInformation* info);

private:
  vtkPMergeConnected(const vtkPMergeConnected&) = delete;
  void operator=(const vtkPMergeConnected&) = delete;

  // parallelism
  int NumProcesses;
  int MyId;
  vtkMultiProcessController* Controller;
  void SetController(vtkMultiProcessController* c);

  // filter
  void LocalToGlobalRegionId(vtkMultiProcessController* contr, vtkMultiBlockDataSet* data);
  void MergeCellsOnRegionId(vtkUnstructuredGrid* ugrid, int target, vtkIdList* facestream);
  float MergeCellDataOnRegionId(
    vtkFloatArray* data_array, vtkIdTypeArray* rid_array, vtkIdType target);

  void delete_key(FaceWithKey* key);
  FaceWithKey* IdsToKey(vtkIdList* ids);
};

#endif
