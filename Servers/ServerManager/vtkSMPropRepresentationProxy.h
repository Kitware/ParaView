/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPropRepresentationProxy
// .SECTION Description
// vtkSMPropRepresentationProxy is superclass for all representations that go in
// the vtkSMRenderViewProxy. It simply collects the common selection code.
// It should not define any API used by vtkSMRenderViewProxy i.e. it should be
// possible for used to write representations which are subclasses of
// vtkSMDataRepresentationProxy itself that go in the vtkSMRenderViewProxy.

#ifndef __vtkSMPropRepresentationProxy_h
#define __vtkSMPropRepresentationProxy_h

#include "vtkSMDataRepresentationProxy.h"

class vtkSMProxyLink;

class VTK_EXPORT vtkSMPropRepresentationProxy : public vtkSMDataRepresentationProxy
{
public:
  static vtkSMPropRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMPropRepresentationProxy, vtkSMDataRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called to update the Representation. 
  // Overridden to ensure that SelectionRepresentation visibility is updated
  // correctly.
  virtual void Update(vtkSMViewProxy* view);
  virtual void Update() { this->Superclass::Update(); };

  // Description:
  // Overridden to pass to SelectionRepresentation.
  virtual void SetUpdateTime(double time);

  // Description:
  // Overridden to fill with strategies from SelectionRepresentation.
  virtual void GetActiveStrategies(
    vtkSMRepresentationStrategyVector& activeStrategies);

  // Description:
  // Set the visibility for this representation.
  // Implemented to turn off selection actor when visibility is turned off.
  virtual void SetVisibility(int visible);
//BTX
protected:
  vtkSMPropRepresentationProxy();
  ~vtkSMPropRepresentationProxy();

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

  // Description:
  // Some properties of the Prop (Actor) proxy need to be shared with the
  // selection prop so that actor tranformations are applied on both.
  void LinkSelectionProp(vtkSMProxy* prop);

  // Selection Representation.
  vtkSMDataRepresentationProxy* SelectionRepresentation;
  vtkSMProxyLink* SelectionPropLink;

private:
  vtkSMPropRepresentationProxy(const vtkSMPropRepresentationProxy&); // Not implemented
  void operator=(const vtkSMPropRepresentationProxy&); // Not implemented
//ETX
};

#endif

