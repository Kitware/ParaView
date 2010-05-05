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

class VTK_EXPORT vtkSMPrismCubeAxesRepresentationProxy : public vtkSMDataRepresentationProxy
{
public:
  static vtkSMPrismCubeAxesRepresentationProxy* New();
  vtkTypeMacro(vtkSMPrismCubeAxesRepresentationProxy, vtkSMDataRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called to update the Representation. 
  // Overridden to gather the bounds from the input and then set them on the
  // CubeAxesActor.
  virtual void Update(vtkSMViewProxy* view);
  virtual void Update() { this->Superclass::Update(); }

  vtkSetVector3Macro(Position, double);
  vtkGetVector3Macro(Position, double);

  vtkSetVector3Macro(Orientation, double);
  vtkGetVector3Macro(Orientation, double);

  vtkSetVector3Macro(Scale, double);
  vtkGetVector3Macro(Scale, double);
 

  vtkSetVector6Macro(CustomBounds, double);
  vtkGetVector6Macro(CustomBounds, double);

  vtkSetVector3Macro(CustomBoundsActive, int);
  vtkGetVector3Macro(CustomBoundsActive, int);



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
  virtual bool InitializeStrategy(vtkSMViewProxy* view);

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

  int CubeAxesVisibility;
  vtkSMSourceProxy* OutlineFilter;
  vtkSMProxy* CubeAxesActor;
  vtkSMProxy* Property;
  vtkSMRepresentationStrategy* Strategy;
  double Position[3], Scale[3], Orientation[3];
  double CustomBounds[6];
  int CustomBoundsActive[3];

private:
  vtkSMPrismCubeAxesRepresentationProxy(const vtkSMPrismCubeAxesRepresentationProxy&); // Not implemented
  void operator=(const vtkSMPrismCubeAxesRepresentationProxy&); // Not implemented
//ETX
};

#endif

