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
// .NAME vtkSMClientDeliveryStrategyProxy - strategy used by client delivery
// representation.
// .SECTION Description
// This strategy delivers data to the client. It is possible to specficy the
// reduction algorithm to use.
// This strategy does not support LOD.

#ifndef __vtkSMClientDeliveryStrategyProxy_h
#define __vtkSMClientDeliveryStrategyProxy_h

#include "vtkSMSimpleStrategy.h"

class VTK_EXPORT vtkSMClientDeliveryStrategyProxy : public vtkSMSimpleStrategy
{
public:
  static vtkSMClientDeliveryStrategyProxy* New();
  vtkTypeMacro(vtkSMClientDeliveryStrategyProxy, vtkSMSimpleStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get the output from the strategy.
  virtual vtkSMSourceProxy* GetOutput()
    { return this->PostCollectUpdateSuppressor; }
  
  // Description:
  // The reduction filter collects data from all processes on the root node and
  // then combines them together using the post gather reduction helper.
  // This method is used to specify the class name for the algorithm to use for
  // this purpose.
  void SetPostGatherHelper(const char* classname);
  void SetPostGatherHelper(vtkSMProxy* helper);
  void SetPreGatherHelper(vtkSMProxy* helper);
//BTX
protected:
  vtkSMClientDeliveryStrategyProxy();
  ~vtkSMClientDeliveryStrategyProxy();

  // Description:
  // Overridden to set the servers correctly on all subproxies.
  virtual void BeginCreateVTKObjects();

  // Description:
  // Create and initialize the data pipeline.
  virtual void CreatePipeline(vtkSMSourceProxy* input, int outputport);

  // Description:
  // Updates the data pipeline (non-LOD only).
  // Overridden to pass correct collection decision to the Collect filter
  // based on UseCompositing() flag.
  virtual void UpdatePipeline();

  // Description:
  // Invalidates the full resolution pipeline.
  virtual void InvalidatePipeline()
    {
    this->Superclass::InvalidatePipeline();
    this->CollectedDataValid = false;
    }

  // Description:
  // Returns true is data is valid.
  virtual bool GetDataValid()
    {
    return this->Superclass::GetDataValid() && this->CollectedDataValid;
    }
  vtkSMSourceProxy* ReductionProxy;
  vtkSMSourceProxy* CollectProxy;
  vtkSMSourceProxy* PostCollectUpdateSuppressor;
  bool CollectedDataValid;
private:
  vtkSMClientDeliveryStrategyProxy(const vtkSMClientDeliveryStrategyProxy&); // Not implemented
  void operator=(const vtkSMClientDeliveryStrategyProxy&); // Not implemented

  // Since LOD and full res pipeline have exactly the same setup, we have this
  // common method.
  void UpdatePipelineInternal(vtkSMSourceProxy* collect,
    vtkSMSourceProxy* updatesuppressor);

//ETX
};

#endif

