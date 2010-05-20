/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTextSourceRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMTextSourceRepresentationProxy
// .SECTION Description
//

#ifndef __vtkSMTextSourceRepresentationProxy_h
#define __vtkSMTextSourceRepresentationProxy_h

#include "vtkSMClientDeliveryRepresentationProxy.h"

class vtkSMTextWidgetRepresentationProxy;
class vtkSMViewProxy;

class VTK_EXPORT vtkSMTextSourceRepresentationProxy : public vtkSMClientDeliveryRepresentationProxy
{
public:
  static vtkSMTextSourceRepresentationProxy* New();
  vtkTypeMacro(vtkSMTextSourceRepresentationProxy, vtkSMClientDeliveryRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the visibility for the representation.
  void SetVisibility(int);

  // Description:
  // Called to update the Display. Default implementation does nothing.
  // Argument is the view requesting the update. Can be null in the
  // case when something other than a view is requesting the update.
  virtual void Update() { this->Update(0); };
  virtual void Update(vtkSMViewProxy* view);

  // Description:
  // Get the bounds for the representation.
  // Overridden to now return any bounds since Text has no bounds.
  virtual bool GetBounds(double*)
    { return false; }

// BTX
protected:
  vtkSMTextSourceRepresentationProxy();
  ~vtkSMTextSourceRepresentationProxy();

  // Description:
  // This method is called at the beginning of CreateVTKObjects().
  // This gives the subclasses an opportunity to set the servers flags
  // on the subproxies.
  // If this method returns false, CreateVTKObjects() is aborted.
  virtual bool BeginCreateVTKObjects();
  virtual bool EndCreateVTKObjects();

  // Description:
  // Called when a representation is added to a view. 
  // Returns true on success.
  // Currently a representation can be added to only one view.
  virtual bool AddToView(vtkSMViewProxy* view);

  // Description:
  // Called to remove a representation from a view.
  // Returns true on success.
  // Currently a representation can be added to only one view.
  virtual bool RemoveFromView(vtkSMViewProxy* view);

  vtkSMTextWidgetRepresentationProxy* TextWidgetProxy;
private:
  vtkSMTextSourceRepresentationProxy(const vtkSMTextSourceRepresentationProxy&); // Not implemented
  void operator=(const vtkSMTextSourceRepresentationProxy&); // Not implemented
//ETX
};

#endif

