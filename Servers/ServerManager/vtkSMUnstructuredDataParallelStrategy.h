/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUnstructuredDataParallelStrategy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMUnstructuredDataParallelStrategy - strategy for unstructured data
// (vtkUnstructuredGrid or vtkPolyData).
// .SECTION Description
// vtkSMUnstructuredDataParallelStrategy is a stategy for unstructured data. It
// extends vtkSMSimpleParallelStrategy by adding support for redistributing data
// if needed (as is the case when ordered compositing is needed).

#ifndef __vtkSMUnstructuredDataParallelStrategy_h
#define __vtkSMUnstructuredDataParallelStrategy_h

#include "vtkSMSimpleParallelStrategy.h"

class VTK_EXPORT vtkSMUnstructuredDataParallelStrategy : public vtkSMSimpleParallelStrategy
{
public:
  static vtkSMUnstructuredDataParallelStrategy* New();
  vtkTypeMacro(vtkSMUnstructuredDataParallelStrategy, vtkSMSimpleParallelStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output from the strategy.
  virtual vtkSMSourceProxy* GetOutput()
    { return this->PostDistributorSuppressor; }

  // Description:
  // Get low resolution output.
  virtual vtkSMSourceProxy* GetLODOutput()
    { return this->PostDistributorSuppressorLOD; }

  // Description:
  // Returns the data generator that goes into the distributor.
  virtual vtkSMSourceProxy* GetDistributedSource()
    { return this->Superclass::GetOutput(); }

  // Description:
  // Update the data the gets distributed.
  virtual void UpdateDistributedData();

  // Description:
  // This is called to mark the distributed data invalid. This must be called
  // whenever the KdTree is rebuilt.
  void InvalidateDistributedData();

//BTX
protected:
  vtkSMUnstructuredDataParallelStrategy();
  ~vtkSMUnstructuredDataParallelStrategy();

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
  // Called to set the KdTree proxy on the distributor.
  void SetKdTree(vtkSMProxy*);

  virtual bool GetDataValid()
    { return this->DistributedDataValid && this->Superclass::GetDataValid(); }

  virtual bool GetLODDataValid()
    { 
    return this->DistributedLODDataValid &&
      this->Superclass::GetLODDataValid(); 
    }

  // Description:
  // Invalidates the LOD pipeline.
  virtual void InvalidateLODPipeline()
    { 
    this->Superclass::InvalidateLODPipeline();
    this->DistributedLODDataValid = false;
    }

  // Description:
  // Invalidates the full resolution pipeline.
  virtual void InvalidatePipeline()
    {
    this->Superclass::InvalidatePipeline();
    this->DistributedDataValid = false;
    }

  bool DistributedDataValid;
  bool DistributedLODDataValid;

  bool UseOrderedCompositing;
  vtkSMProxy* KdTree;

  vtkSMSourceProxy* PostDistributorSuppressor;
  vtkSMSourceProxy* Distributor;

  vtkSMSourceProxy* PostDistributorSuppressorLOD;
  vtkSMSourceProxy* DistributorLOD;

private:
  vtkSMUnstructuredDataParallelStrategy(const vtkSMUnstructuredDataParallelStrategy&); // Not implemented
  void operator=(const vtkSMUnstructuredDataParallelStrategy&); // Not implemented

  void CreatePipelineInternal(vtkSMSourceProxy* input,
    vtkSMSourceProxy* distributor, vtkSMSourceProxy* updatesuppressor);

//ETX
};

#endif

