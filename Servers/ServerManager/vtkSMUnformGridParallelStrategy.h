/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUnformGridParallelStrategy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMUnformGridParallelStrategy
// .SECTION Description
//

#ifndef __vtkSMUnformGridParallelStrategy_h
#define __vtkSMUnformGridParallelStrategy_h

#include "vtkSMSimpleStrategy.h"

class VTK_EXPORT vtkSMUnformGridParallelStrategy : public vtkSMSimpleStrategy
{
public:
  static vtkSMUnformGridParallelStrategy* New();
  vtkTypeRevisionMacro(vtkSMUnformGridParallelStrategy, vtkSMSimpleStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMUnformGridParallelStrategy();
  ~vtkSMUnformGridParallelStrategy();

  // Description:
  // Overridden to set the servers correctly on all subproxies.
  virtual void CreateVTKObjects();

  // Description:
  // Create and initialize the data pipeline.
  virtual void CreatePipeline(vtkSMSourceProxy* input);

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

  vtkSMProxy* Collect;

private:
  vtkSMUnformGridParallelStrategy(const vtkSMUnformGridParallelStrategy&); // Not implemented
  void operator=(const vtkSMUnformGridParallelStrategy&); // Not implemented
//ETX
};

#endif

