/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLODActor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVLODActor - an actor that supports multiple levels of detail
// .SECTION Description
// vtkPVLODActor  is a very simple version of vtkLODActor.  Both
// vtkLODActor and vtkLODProp3D can get confused, and substitute
// LOD mappers when they are not needed.  This just has two mappers:
// full res and LOD, and this actor knows which is which.

// .SECTION see also
// vtkActor vtkRenderer vtkLODProp3D vtkLODActor

#ifndef __vtkPVLODActor_h
#define __vtkPVLODActor_h

#include "vtkActor.h"

class vtkMapper;

class VTK_EXPORT vtkPVLODActor : public vtkActor
{
public:
  vtkTypeMacro(vtkPVLODActor,vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPVLODActor *New();

  // Description:
  // This causes the actor to be rendered. It, in turn, will render the actor's
  // property and then mapper.  
  virtual void Render(vtkRenderer *, vtkMapper *);

  // Description:
  // This method is used internally by the rendering process.
  // We overide the superclass method to properly set the estimated render time.
  int RenderOpaqueGeometry(vtkViewport *viewport);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // This sets the low res input.
  virtual void SetLODMapper(vtkMapper*);
  vtkGetObjectMacro(LODMapper, vtkMapper);

  // Description:
  // This is a bit of a hack.  This returns the last mapper used to render.
  // It does this so that compositing can descide if anything was actually renderered.
  vtkMapper *GetMapper() {return this->SelectMapper();}

  // Description:
  // When this objects gets modified, this method also modifies the object.
  void Modified();
  
  // Description:
  // Shallow copy of an LOD actor. Overloads the virtual vtkProp method.
  void ShallowCopy(vtkProp *prop);

  // Description:
  // Get the bounds of the current mapper.
  double *GetBounds();

  // Description:
  // When set, LODMapper, if present it used, otherwise the regular mapper is
  // used.
  vtkSetMacro(EnableLOD, int);
  vtkGetMacro(EnableLOD, int);

protected:
  vtkPVLODActor();
  ~vtkPVLODActor();
  vtkActor            *Device;
  vtkMapper           *LODMapper;

  vtkMapper *SelectMapper();

  int EnableLOD;

private:
  vtkPVLODActor(const vtkPVLODActor&); // Not implemented.
  void operator=(const vtkPVLODActor&); // Not implemented.
};

#endif


