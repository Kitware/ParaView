/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCubeAxesRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCubeAxesRepresentationProxy - representation proxy for CubeAxes.
// .SECTION Description
// vtkSMCubeAxesRepresentationProxy can be used to show a bounding cube axes to
// any dataset.

#ifndef __vtkSMCubeAxesRepresentationProxy_h
#define __vtkSMCubeAxesRepresentationProxy_h

#include "vtkSMDataRepresentationProxy.h"

class VTK_EXPORT vtkSMCubeAxesRepresentationProxy : public vtkSMDataRepresentationProxy
{
public:
  static vtkSMCubeAxesRepresentationProxy* New();
  vtkTypeMacro(vtkSMCubeAxesRepresentationProxy, vtkSMDataRepresentationProxy);
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

//BTX
protected:
  vtkSMCubeAxesRepresentationProxy();
  ~vtkSMCubeAxesRepresentationProxy();

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

  vtkSMSourceProxy* OutlineFilter;
  vtkSMProxy* CubeAxesActor;
  vtkSMProxy* Property;
  vtkSMRepresentationStrategy* Strategy;
  double Position[3], Scale[3], Orientation[3];
  double CustomBounds[6];
  int CustomBoundsActive[3];
private:
  vtkSMCubeAxesRepresentationProxy(const vtkSMCubeAxesRepresentationProxy&); // Not implemented
  void operator=(const vtkSMCubeAxesRepresentationProxy&); // Not implemented
//ETX
};

#endif

