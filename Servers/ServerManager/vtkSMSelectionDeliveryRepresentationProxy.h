/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSelectionDeliveryRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSelectionDeliveryRepresentationProxy - representation that can be
// used to show client delivery displays, such as a LineChart in a render view.
// .SECTION Description
// vtkSMSelectionDeliveryRepresentationProxy is a representation that can be used
// to show displays that needs client delivery, such as a LineChart, etc.
// It also delivers the selection to the client.

#ifndef __vtkSMSelectionDeliveryRepresentationProxy_h
#define __vtkSMSelectionDeliveryRepresentationProxy_h

#include "vtkSMClientDeliveryRepresentationProxy.h"

class VTK_EXPORT vtkSMSelectionDeliveryRepresentationProxy : 
  public vtkSMClientDeliveryRepresentationProxy
{
public:
  static vtkSMSelectionDeliveryRepresentationProxy* New();
  vtkTypeMacro(vtkSMSelectionDeliveryRepresentationProxy, vtkSMClientDeliveryRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkGetObjectMacro(SelectionRepresentation, vtkSMClientDeliveryRepresentationProxy);

  // Description:
  // Called to update the Display. Default implementation does nothing.
  // Argument is the view requesting the update. Can be null in the
  // case when something other than a view is requesting the update.
  // Overridden to update the internal SelectionRepresentation proxy as well. 
  virtual void Update() { this->Superclass::Update(); };
  virtual void Update(vtkSMViewProxy* view);

//BTX
protected:
  vtkSMSelectionDeliveryRepresentationProxy();
  ~vtkSMSelectionDeliveryRepresentationProxy();

  // Description:
  // This method is called at the beginning of CreateVTKObjects().
  // If this method returns false, CreateVTKObjects() is aborted.
  // Overridden to abort CreateVTKObjects() only if the input has
  // been initialized correctly.
  virtual bool BeginCreateVTKObjects();

  // Description:
  // Create the data pipeline.
  virtual void CreatePipeline(vtkSMSourceProxy* input, int outputport);

  vtkSMClientDeliveryRepresentationProxy* SelectionRepresentation;

private:
  vtkSMSelectionDeliveryRepresentationProxy(const vtkSMSelectionDeliveryRepresentationProxy&); // Not implemented
  void operator=(const vtkSMSelectionDeliveryRepresentationProxy&); // Not implemented
//ETX
};

#endif

