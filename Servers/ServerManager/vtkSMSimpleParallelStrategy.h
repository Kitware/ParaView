/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSimpleParallelStrategy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSimpleParallelStrategy
// .SECTION Description
// vtkSMSimpleParallelStrategy is a representation used by parallel render
// views.

#ifndef __vtkSMSimpleParallelStrategy_h
#define __vtkSMSimpleParallelStrategy_h

#include "vtkSMSimpleStrategy.h"

class VTK_EXPORT vtkSMSimpleParallelStrategy : public vtkSMSimpleStrategy
{
public:
  static vtkSMSimpleParallelStrategy* New();
  vtkTypeRevisionMacro(vtkSMSimpleParallelStrategy, vtkSMSimpleStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the data generator that goes into the distributor.
  virtual vtkSMSourceProxy* GetDistributedSource()
    { return this->PreDistributorSuppressor; }

  // Description:
  // Update the data the gets distributed.
  virtual void UpdateDistributedData();
//BTX
protected:
  vtkSMSimpleParallelStrategy();
  ~vtkSMSimpleParallelStrategy();

  // Description:
  // Overridden to set the servers correctly on all subproxies.
  virtual void BeginCreateVTKObjects();
  virtual void EndCreateVTKObjects();

  // Description:
  // Create and initialize the data pipeline.
  virtual void CreatePipeline(vtkSMSourceProxy* input, int outputport);

  // Description:
  // Create and initialize the LOD data pipeline.
  // Note that this method is called irrespective of EnableLOD
  // flag.
  virtual void CreateLODPipeline(vtkSMSourceProxy* input, int outputport);

  // Description:
  // Gather the information of the displayed data
  // for the current update state of the data pipeline (non-LOD).
  // Overridden to update the server flag to ensure that the information is
  // collected from  the update suppressor that has valid data depending upon
  // the state of compositing flag.
  virtual void GatherInformation(vtkPVDataInformation*);

  // Description:
  // Gather the information of the displayed data
  // for the current update state of the LOD pipeline.
  // Overridden to update the server flag to ensure that the information is
  // collected from  the update suppressor that has valid data depending upon
  // the state of compositing flag.
  virtual void GatherLODInformation(vtkPVDataInformation*);

  // Description:
  // Update the LOD pipeline.
  // Overridden to pass correct collection decision to the Collect filter
  // based on UseCompositing flag.
  virtual void UpdateLODPipeline();

  // Description:
  // Updates the data pipeline (non-LOD only).
  // Overridden to pass correct collection decision to the Collect filter
  // based on UseCompositing flag.
  virtual void UpdatePipeline();

  // Description:
  // Called when ever the view information changes.
  // The strategy should update it's state based on the current view information
  // provided the information object.
  virtual void ProcessViewInformation();

  // Description:
  // Update the distributor to use ordered compositing.
  // Called in ViewHelperModified.
  vtkGetMacro(UseOrderedCompositing, bool);
  virtual void SetUseOrderedCompositing(bool);

  // Description:
  // Called by the view to make the strategy use compositing.
  // Called in ProcessViewInformation.
  void SetUseCompositing(bool);
  vtkGetMacro(UseCompositing, bool);

  // Description:
  // Called to set the KdTree proxy on the distributor.
  void SetKdTree(vtkSMProxy*);

  void SetLODClientCollect(bool);
  void SetLODClientRender(bool);

  vtkSMSourceProxy* Collect;
  vtkSMSourceProxy* PreDistributorSuppressor;
  vtkSMSourceProxy* Distributor;

  vtkSMSourceProxy* CollectLOD;
  vtkSMSourceProxy* PreDistributorSuppressorLOD;
  vtkSMSourceProxy* DistributorLOD;

  vtkSMProxy* KdTree;
  
  bool UseOrderedCompositing;
  bool UseCompositing;
  bool LODClientRender; // when set, indicates that data must be made available on client,
                     // irrespective of UseCompositing flag.

  bool LODClientCollect; // When set, the data delivered to client is outline of the data
                      // not the whole data.
private:
  vtkSMSimpleParallelStrategy(const vtkSMSimpleParallelStrategy&); // Not implemented
  void operator=(const vtkSMSimpleParallelStrategy&); // Not implemented

  // Since LOD and full res pipeline have exactly the same setup, we have this
  // common method.
  void CreatePipelineInternal(vtkSMSourceProxy* input, int outputport,
    vtkSMSourceProxy* collect,
    vtkSMSourceProxy* predistributorsuppressor, vtkSMSourceProxy* distributor,
    vtkSMSourceProxy* updatesuppressor);
//ETX
};

#endif

