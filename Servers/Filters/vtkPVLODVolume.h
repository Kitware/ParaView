/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLODVolume.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVLODVolume - an actor that supports multiple levels of detail
// .SECTION Description
// vtkPVLODVolume is much like vtkPVLODActor except that it works on
// volumes instead of surfaces.  This just has two mappers: full res and
// LOD, and this actor knows which is which.

// .SECTION see also
// vtkActor vtkRenderer vtkLODProp3D vtkLODActor

#ifndef __vtkPVLODVolume_h
#define __vtkPVLODVolume_h

#include "vtkVolume.h"

class vtkAbstractVolumeMapper;

class VTK_EXPORT vtkPVLODVolume : public vtkVolume
{
public:
  vtkTypeRevisionMacro(vtkPVLODVolume,vtkVolume);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPVLODVolume *New();

  // Description:
  // This method is used internally by the rendering process.
  virtual int RenderTranslucentGeometry(vtkViewport *viewport);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // This sets the low res input.
  virtual void SetLODMapper(vtkAbstractVolumeMapper*);
  vtkGetObjectMacro(LODMapper, vtkAbstractVolumeMapper);

  // Description:
  // This is a bit of a hack.  This returns the last mapper used to render.
  // It does this so that compositing can descide if anything was actually
  // renderered.
  virtual vtkAbstractVolumeMapper *GetMapper() {return this->SelectMapper();}

  // Description:
  // Shallow copy of an LOD actor. Overloads the virtual vtkProp method.
  virtual void ShallowCopy(vtkProp *prop);

  // Description:
  // Get the bounds of the current mapper.
  virtual double *GetBounds();

protected:
  vtkPVLODVolume();
  ~vtkPVLODVolume();
  vtkAbstractVolumeMapper *LODMapper;

  vtkAbstractVolumeMapper *SelectMapper();

  double MapperBounds[6];
  vtkTimeStamp BoundsMTime;

private:
  vtkPVLODVolume(const vtkPVLODVolume&); // Not implemented.
  void operator=(const vtkPVLODVolume&); // Not implemented.
};

#endif


