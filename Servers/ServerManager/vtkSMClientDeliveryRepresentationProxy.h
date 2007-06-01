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

class VTK_EXPORT vtkSMClientDeliveryRepresentationProxy : 
  public vtkSMDataRepresentationProxy
{
public:
  static vtkSMClientDeliveryRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMClientDeliveryRepresentationProxy, vtkSMDataRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Method gets called to set input when using Input property.
  // Internally leads to a call to SetInput.
  virtual void AddInput(vtkSMSourceProxy* input, const char* method,
    int hasMultipleInputs);

  // Description:
  // Get the data that was collected to the client
  virtual vtkDataObject* GetOutput();

  //BTX
  enum ReductionTypeEnum
    {
    ADD = 0,
    POLYDATA_APPEND = 1,
    UNSTRUCTURED_APPEND = 2,
    FIRST_NODE_ONLY = 3,
    RECTILINEAR_GRID_APPEND=4
    };
  //ETX

  // Description:
  // Set the reduction algorithm type. Cannot be called before
  // objects are created.
  void SetReductionType(int type);

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

  void SetInputInternal();
  bool SetupStrategy();

  vtkSMProxy* ReduceProxy;
  vtkSMRepresentationStrategy* StrategyProxy;

  int ReductionType;

  // Proxies for the selection pipeline.
  vtkSMSourceProxy* ExtractSelection;

private:
  vtkSMClientDeliveryRepresentationProxy(const vtkSMClientDeliveryRepresentationProxy&); // Not implemented
  void operator=(const vtkSMClientDeliveryRepresentationProxy&); // Not implemented
//ETX
};

#endif

