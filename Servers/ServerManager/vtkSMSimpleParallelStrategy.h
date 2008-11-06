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

//BTX
protected:
  vtkSMSimpleParallelStrategy();
  ~vtkSMSimpleParallelStrategy();

  // Description:
  // Overridden to set the servers correctly on all subproxies.
  virtual void BeginCreateVTKObjects();

  // Description:
  // Create and initialize the data pipeline.
  virtual void CreatePipeline(vtkSMSourceProxy* input, int outputport);

  // Description:
  // Create and initialize the LOD data pipeline.
  // Note that this method is called irrespective of EnableLOD
  // flag.
  virtual void CreateLODPipeline(vtkSMSourceProxy* input, int outputport);

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
  // Called by the view to make the strategy use compositing.
  // Called in ProcessViewInformation.
  void SetUseCompositing(bool);
  vtkGetMacro(UseCompositing, bool);

  void SetLODClientCollect(bool);
  void SetLODClientRender(bool);

  // Since LOD and full res pipeline have exactly the same setup, we have this
  // common method. //DDM TODO Why public?
  void CreatePipelineInternal(vtkSMSourceProxy* input, int outputport,
    vtkSMSourceProxy* collect,
    vtkSMSourceProxy* updatesuppressor);

  vtkSMSourceProxy* PreCollectUpdateSuppressor;
  vtkSMSourceProxy* Collect;

  vtkSMSourceProxy* PreCollectUpdateSuppressorLOD;
  vtkSMSourceProxy* CollectLOD;
  
  // In client-server (or parallel) we want to avoid data-movement when caching,
  // hence we use the PostCollectCacheKeeper.
  // This is directly liked to the CacheKeeper (using property linking).
  vtkSMSourceProxy* PostCollectCacheKeeper;

  bool UseCompositing;
  bool LODClientRender; // when set, indicates that data must be made available on client,
                     // irrespective of UseCompositing flag.

  bool LODClientCollect; // When set, the data delivered to client is outline of the data
                      // not the whole data.

private:
  vtkSMSimpleParallelStrategy(const vtkSMSimpleParallelStrategy&); // Not implemented
  void operator=(const vtkSMSimpleParallelStrategy&); // Not implemented

//ETX
};

#endif

