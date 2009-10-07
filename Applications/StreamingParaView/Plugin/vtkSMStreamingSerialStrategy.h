/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStreamingSerialStrategy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMStreamingSerialStrategy
// .SECTION Description
//

#ifndef __vtkSMStreamingSerialStrategy_h
#define __vtkSMStreamingSerialStrategy_h

#include "vtkSMSimpleStrategy.h"

class VTK_EXPORT vtkSMStreamingSerialStrategy : public vtkSMSimpleStrategy
{
public:
  static vtkSMStreamingSerialStrategy* New();
  vtkTypeRevisionMacro(vtkSMStreamingSerialStrategy, vtkSMSimpleStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // LOD and streaming are not working yet.
  virtual void SetEnableLOD(bool vtkNotUsed(enable))
  {}

  // Description:
  // Tells server side to work with a particular piece until further notice.
  virtual void SetPassNumber(int Pass, int force);
  // Description:
  // Orders the pieces from most to least important.
  virtual int ComputePriorities();
  // Description:
  // Copies the piece ordering to dest via serialization.
  virtual void SharePieceList(vtkSMRepresentationStrategy *dest);
  // Description:
  // Clears the data object cache in the streaming display pipeline.
  virtual void ClearStreamCache();

  // Description:
  // Tells the strategy where the camera is so that pieces can be sorted and rejected
  virtual void SetViewState(double *camera, double *frustum);

//BTX
protected:
  vtkSMStreamingSerialStrategy();
  ~vtkSMStreamingSerialStrategy();

  // Description:
  // Copies ordered piece list from one UpdateSupressor to the other.
  virtual void CopyPieceList(vtkClientServerStream *stream,
                             vtkSMSourceProxy *src, 
                             vtkSMSourceProxy *dest);

  // Description:
  // Overridden to set the servers correctly on all subproxies.
  virtual void BeginCreateVTKObjects();

  // Description:
  // Create and initialize the data pipeline.
  virtual void CreatePipeline(vtkSMSourceProxy* input, int outputport);
  
  // Description:
  // Gather the information of the displayed data (non-LOD).
  // Update the part of the pipeline needed to gather full information
  // and then gather that information. 
  virtual void GatherInformation(vtkPVInformation*);

  // Description:
  // Invalidates the full resolution pipeline, overridden to clean up cache.
  virtual void InvalidatePipeline();

  vtkSMSourceProxy* PieceCache;
  vtkSMSourceProxy* ViewSorter;

private:
  vtkSMStreamingSerialStrategy(const vtkSMStreamingSerialStrategy&); // Not implemented
  void operator=(const vtkSMStreamingSerialStrategy&); // Not implemented
//ETX
};

#endif

