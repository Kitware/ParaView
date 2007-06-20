/*=========================================================================

  Program:   ParaView
  Module:    vtkSMClientDeliveryStrategyProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMClientDeliveryStrategyProxy
// .SECTION Description
//

#ifndef __vtkSMClientDeliveryStrategyProxy_h
#define __vtkSMClientDeliveryStrategyProxy_h

#include "vtkSMSimpleStrategy.h"

class VTK_EXPORT vtkSMClientDeliveryStrategyProxy : public vtkSMSimpleStrategy
{
public:
  static vtkSMClientDeliveryStrategyProxy* New();
  vtkTypeRevisionMacro(vtkSMClientDeliveryStrategyProxy, vtkSMSimpleStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get the output from the strategy.
  virtual vtkSMSourceProxy* GetOutput();

//BTX
protected:
  vtkSMClientDeliveryStrategyProxy();
  ~vtkSMClientDeliveryStrategyProxy();

  // Description:
  // Overridden to set the servers correctly on all subproxies.
  virtual void BeginCreateVTKObjects();

  // Description:
  // Create and initialize the data pipeline.
  virtual void CreatePipeline(vtkSMSourceProxy* input);

  // Description:
  // Create and initialize the LOD data pipeline.
  // Note that this method is called irrespective of EnableLOD
  // flag.
  virtual void CreateLODPipeline(vtkSMSourceProxy* input);

  // Description:
  // Update the LOD pipeline.
  // Overridden to pass correct collection decision to the Collect filter
  // based on UseCompositing() flag.
  virtual void UpdateLODPipeline();

  // Description:
  // Updates the data pipeline (non-LOD only).
  // Overridden to pass correct collection decision to the Collect filter
  // based on UseCompositing() flag.
  virtual void UpdatePipeline();

  vtkSMSourceProxy* CollectProxy;
  vtkSMSourceProxy* CollectLODProxy;
  
private:
  vtkSMClientDeliveryStrategyProxy(const vtkSMClientDeliveryStrategyProxy&); // Not implemented
  void operator=(const vtkSMClientDeliveryStrategyProxy&); // Not implemented

  // Since LOD and full res pipeline have exactly the same setup, we have this
  // common method.
  void CreatePipelineInternal(vtkSMSourceProxy* input, vtkSMSourceProxy* collect,
    vtkSMSourceProxy* updatesuppressor);
  void UpdatePipelineInternal(vtkSMSourceProxy* collect,
    vtkSMSourceProxy* updatesuppressor);

//ETX
};

#endif

