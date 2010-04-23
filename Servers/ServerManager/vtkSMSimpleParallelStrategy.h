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
  vtkTypeMacro(vtkSMSimpleParallelStrategy, vtkSMSimpleStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output from the strategy.
  virtual vtkSMSourceProxy* GetOutput()
    { return this->PostCollectUpdateSuppressor; }

  // Description:
  // Get low resolution output.
  virtual vtkSMSourceProxy* GetLODOutput()
    { return this->PostCollectUpdateSuppressorLOD; }

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
  // Invalidates the LOD pipeline.
  virtual void InvalidateLODPipeline()
    { 
    this->Superclass::InvalidateLODPipeline();
    this->CollectedLODDataValid = false;
    }

  // Description:
  // Invalidates the full resolution pipeline.
  virtual void InvalidatePipeline()
    {
    this->Superclass::InvalidatePipeline();
    this->CollectedDataValid = false;
    }

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

  // Description:
  // When LODClientCollect is on (default) it implies than when rendering
  // Low-res, if the compositing decision is to deliver to client, then it's
  // acceptable to deliver the LOD data to client. If set to false, only the
  // outline will be delivered to the client.
  void SetLODClientCollect(bool);

  // Description:
  // This flag is set when rendering in tile-display mode. It implies that LOD
  // data must be rendered on the client side. TODO: this flag's name needs to
  // be fixed to be more sensible. 
  void SetLODClientRender(bool);

  virtual bool GetDataValid()
    { 
    return this->CollectedDataValid && this->Superclass::GetDataValid();
    }

  virtual bool GetLODDataValid()
    { 
    return this->CollectedLODDataValid &&
      this->Superclass::GetLODDataValid(); 
    }

  // Description:
  // Determine where the data is to be delivered for rendering the current
  // frame.
  virtual int GetMoveMode();
  virtual int GetLODMoveMode();

  // Since LOD and full res pipeline have exactly the same setup, we have this
  // common method. //DDM TODO Why public?
  void CreatePipelineInternal(vtkSMSourceProxy* input, int outputport,
    vtkSMSourceProxy* collect,
    vtkSMSourceProxy* updatesuppressor);

  vtkSMSourceProxy* Collect;
  vtkSMSourceProxy* PostCollectUpdateSuppressor;

  vtkSMSourceProxy* CollectLOD;
  vtkSMSourceProxy* PostCollectUpdateSuppressorLOD;
  
  // In client-server (or parallel) we want to avoid data-movement when caching,
  // hence we use the PostCollectCacheKeeper.
  // This is directly liked to the CacheKeeper (using property linking).
  vtkSMSourceProxy* PostCollectCacheKeeper;

  bool CollectedDataValid;
  bool CollectedLODDataValid;
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

