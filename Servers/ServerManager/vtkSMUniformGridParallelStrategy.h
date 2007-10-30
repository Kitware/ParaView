/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUniformGridParallelStrategy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMUniformGridParallelStrategy
// .SECTION Description
// vtkSMUniformGridParallelStrategy is used for rendering image data in
// parallel. 
//
// This strategy does not support LOD pipeline in the generic sense, LOD for
// volume rendering is managed by the mapper itself.
//
// To support client side rendering of the data, this strategy overloads the LOD
// pipeline. LOD pipeline now simply means a client-side pipeline that delivers
// the outline of the original data to the client.
// vtkSMUniformGridVolumeRepresentationProxy works with this to always use
// LODOutput from the strategy when rendering on client.

#ifndef __vtkSMUniformGridParallelStrategy_h
#define __vtkSMUniformGridParallelStrategy_h

#include "vtkSMSimpleStrategy.h"

class VTK_EXPORT vtkSMUniformGridParallelStrategy : public vtkSMSimpleStrategy
{
public:
  static vtkSMUniformGridParallelStrategy* New();
  vtkTypeRevisionMacro(vtkSMUniformGridParallelStrategy, vtkSMSimpleStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns if the strategy is currently using LOD 
  // This strategy never uses LOD for level-of-detail. It is used as client side
  // data when rendering locally.
  bool GetUseLOD()
    { return false; }

  virtual unsigned long GetLODMemorySize()
    { return this->GetFullResMemorySize(); }
//BTX
protected:
  vtkSMUniformGridParallelStrategy();
  ~vtkSMUniformGridParallelStrategy();

  // Description:
  // Overridden to set the servers correctly on all subproxies.
  virtual void BeginCreateVTKObjects();
  virtual void EndCreateVTKObjects();

  // Description:
  // Create and initialize the data pipeline.
  virtual void CreatePipeline(vtkSMSourceProxy* input, int outputport);
  virtual void CreateLODPipeline(vtkSMSourceProxy* input, int outputport);

  // Description:
  // Overridden to avoid unnecessary LOD information collection.
  virtual void GatherLODInformation(vtkPVInformation*) {}

  void InitializeCollectProxy(vtkSMProxy* collect);
  vtkSMSourceProxy* Collect;
  vtkSMSourceProxy* CollectLOD;
private:
  vtkSMUniformGridParallelStrategy(const vtkSMUniformGridParallelStrategy&); // Not implemented
  void operator=(const vtkSMUniformGridParallelStrategy&); // Not implemented
//ETX
};

#endif

