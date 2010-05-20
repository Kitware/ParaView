/*=========================================================================

  Program:   ParaView
  Module:    vtkSMClientDeliveryRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMClientDeliveryRepresentationProxy - representation that can be used to
// show client delivery displays, such as a LineChart in a render view.
// .SECTION Description
// vtkSMClientDeliveryRepresentationProxy is a representation that can be used
// to show displays that needs client delivery, such as a LineChart, etc.

#ifndef __vtkSMClientDeliveryRepresentationProxy_h
#define __vtkSMClientDeliveryRepresentationProxy_h

#include "vtkSMDataRepresentationProxy.h"

class vtkDataObject;
class vtkAlgorithmOutput;
class vtkSMClientDeliveryStrategyProxy;

class VTK_EXPORT vtkSMClientDeliveryRepresentationProxy : 
  public vtkSMDataRepresentationProxy
{
public:
  static vtkSMClientDeliveryRepresentationProxy* New();
  vtkTypeMacro(vtkSMClientDeliveryRepresentationProxy, vtkSMDataRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the data that was collected to the client
  virtual vtkDataObject* GetOutput();

  // Description:
  // Returns the client side output port for the algorithm producing the output
  // returned by GetOutput().
  virtual vtkAlgorithmOutput* GetOutputPort();

  // Description:
  // Called to update the Display. Default implementation does nothing.
  // Argument is the view requesting the update. Can be null in the
  // case when something other than a view is requesting the update.
  virtual void Update() { this->Superclass::Update(); };
  virtual void Update(vtkSMViewProxy* view);

  //BTX
  enum ReductionTypeEnum
    {
    ADD = 0,
    POLYDATA_APPEND = 1,
    UNSTRUCTURED_APPEND = 2,
    FIRST_NODE_ONLY = 3,
    RECTILINEAR_GRID_APPEND=4,
    COMPOSITE_DATASET_APPEND=5,
    CUSTOM=6,
    MULTIBLOCK_MERGE=7,
    TABLE_MERGE=8
    };
  //ETX

  // Description:
  // Set the reduction algorithm type. Cannot be called before
  // objects are created.
  void SetReductionType(int type);

  // Description:
  // Post/Pre gather helper proxy used when ReductionType is CUSTOM,
  // otherwise the helper is automatically determined.
  void SetPostGatherHelper(vtkSMProxy*);
  void SetPreGatherHelper(vtkSMProxy*);

  // Description:
  // Forwards to the representation strategy (ReductionFilter).
  void SetPassThrough(int);

  // Description:
  // Forwards to the representation strategy (ReductionFilter).
  void SetGenerateProcessIds(int);

//BTX
protected:
  vtkSMClientDeliveryRepresentationProxy();
  ~vtkSMClientDeliveryRepresentationProxy();

  // Description:
  // This method is called at the beginning of CreateVTKObjects().
  // This gives the subclasses an opportunity to set the servers flags
  // on the subproxies.
  // If this method returns false, CreateVTKObjects() is aborted.
  virtual bool BeginCreateVTKObjects();

  // Description:
  // This method is called after CreateVTKObjects(). 
  // This gives subclasses an opportunity to do some post-creation
  // initialization.
  virtual bool EndCreateVTKObjects();

  // Description;
  // Create the strategy proxy.
  virtual bool SetupStrategy(vtkSMSourceProxy* input, int outputport);

  // Description:
  // Create the data pipeline.
  virtual void CreatePipeline(vtkSMSourceProxy* input, int outputport);

  vtkSMSourceProxy* PreProcessorProxy;
  vtkSMClientDeliveryStrategyProxy* StrategyProxy;
  vtkSMSourceProxy* PostProcessorProxy;
  vtkSMProxy* PreGatherHelper;
  vtkSMProxy* PostGatherHelper;

  int ReductionType;

private:
  vtkSMClientDeliveryRepresentationProxy(const vtkSMClientDeliveryRepresentationProxy&); // Not implemented
  void operator=(const vtkSMClientDeliveryRepresentationProxy&); // Not implemented
//ETX
};

#endif

