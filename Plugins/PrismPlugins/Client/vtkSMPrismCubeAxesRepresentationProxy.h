/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPrismCubeAxesRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPrismCubeAxesRepresentationProxy - representation proxy for CubeAxes.
// .SECTION Description
// vtkSMPrismCubeAxesRepresentationProxy can be used to show a bounding cube axes to
// any dataset.

#ifndef __vtkSMPrismCubeAxesRepresentationProxy_h
#define __vtkSMPrismCubeAxesRepresentationProxy_h

#include "vtkSMDataRepresentationProxy.h"

class vtkSMPrismCubeAxesRepresentationProxy : public vtkSMDataRepresentationProxy
{
public:
  static vtkSMPrismCubeAxesRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMPrismCubeAxesRepresentationProxy, vtkSMDataRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called to update the Representation. 
  // Overridden to gather the bounds from the input and then set them on the
  // CubeAxesActor.
  virtual void Update(vtkSMViewProxy* view);
  virtual void Update() { this->Superclass::Update(); }



 // Description:
  // Set cube axes visibility. This flag is considered only if
  // this->GetVisibility() == true, otherwise, cube axes is not shown.
  void SetCubeAxesVisibility(int);
  vtkGetMacro(CubeAxesVisibility, int);


//BTX
protected:
  vtkSMPrismCubeAxesRepresentationProxy();
  ~vtkSMPrismCubeAxesRepresentationProxy();

  // Description:
  // This representation needs a surface compositing strategy.
  // Overridden to request the correct type of strategy from the view.
  virtual bool InitializeStrategy(vtkSMViewProxy* vtkNotUsed(view));

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

  vtkSMSourceProxy* OutlineFilter;
  vtkSMProxy* CubeAxesProxy;
  vtkSMProxy* Property;
  vtkSMRepresentationStrategy* Strategy;
  int CubeAxesVisibility;

private:



  vtkSMPrismCubeAxesRepresentationProxy(const vtkSMPrismCubeAxesRepresentationProxy&); // Not implemented
  void operator=(const vtkSMPrismCubeAxesRepresentationProxy&); // Not implemented
//ETX
};

#endif

