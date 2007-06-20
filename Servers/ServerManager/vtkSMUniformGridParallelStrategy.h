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
// parallel. This strategy does not support any LOD pipeline.
// For image data, LOD is managed by the mapper itself.
// Another thing to note about this strategy is that it cannot deliver data to
// client for rendering. Hence it does not worry about the UseCompositing flag
// set by the view.

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
  // Overridden to raise error when someone tries to set it to true. This
  // strategy does not support LOD.
  virtual void SetEnableLOD(bool enable)
    {
    if (enable)
      {
      vtkErrorMacro("This strategy does not support LOD pipelines.");
      }
    else
      {
      this->Superclass::SetEnableLOD(enable);
      }
    }

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
  virtual void CreatePipeline(vtkSMSourceProxy* input);

  vtkSMSourceProxy* Collect;
private:
  vtkSMUniformGridParallelStrategy(const vtkSMUniformGridParallelStrategy&); // Not implemented
  void operator=(const vtkSMUniformGridParallelStrategy&); // Not implemented
//ETX
};

#endif

