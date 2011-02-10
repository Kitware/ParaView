/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCaveSynchronizedRenderers - vtkSynchronizedRenderers subclass that
// handles adjusting of camera for cave configurations.
// .SECTION Description
// vtkCaveSynchronizedRenderers is vtkSynchronizedRenderers used for in CAVE
// configuration. It is used on the render-server side. It ensures that the
// camera is transformed based on the orientations specified in  the pvx
// configuration file.
// This code was previously in class vtkCaveRenderManager.

#ifndef __vtkCaveSynchronizedRenderers_h
#define __vtkCaveSynchronizedRenderers_h

#include "vtkSynchronizedRenderers.h"

class vtkCamera;
class vtkMatrix4x4;

class VTK_EXPORT vtkCaveSynchronizedRenderers : public vtkSynchronizedRenderers
{
public:
  static vtkCaveSynchronizedRenderers* New();
  vtkTypeMacro(vtkCaveSynchronizedRenderers, vtkSynchronizedRenderers);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkCaveSynchronizedRenderers();
  ~vtkCaveSynchronizedRenderers();

  // Description:
  // Called at the start of each render. Overridden to ensure that the camera is
  // updated based on the configuration.
  virtual void HandleStartRender();

  // Description:
  // During constructor, processes the configuration file to setup the display
  // information.
  void ProcessCaveConfiguration();

  // Description:
  // These are to initialize the displays. (This is directly copied from
  // vtkCaveRenderManager).
  void SetNumberOfDisplays(int numberOfDisplays);
  void DefineDisplay(int idx, double origin[3], double x[3], double y[3]);


  // Description:
  // Method to update the camera.
  void ComputeCamera(vtkCamera* cam);

  // Description:
  // This method is used to configure the display at startup. The
  // display is only configurable if the head tracking is set. The
  // typical use case is a CAVE like VR setting and we would like the
  // head-tracked camera to be aware of the display in the room
  // coordinates.
  void SetDisplayConfig();

  // Description:
  // This sets the SurfaceRot transfromation based on the screen
  // basis vectors
  void SetSurfaceRotation( double xBase[3], double yBase[3], double zBase[3]);


  int    NumberOfDisplays;
  double **Displays;
  double DisplayOrigin[4];
  double DisplayX[4];
  double DisplayY[4];
  vtkMatrix4x4 *SurfaceRot;
  double O2Screen;
  double O2Right;
  double O2Left;
  double O2Top;
  double O2Bottom;
  int once;

private:
  vtkCaveSynchronizedRenderers(const vtkCaveSynchronizedRenderers&); // Not implemented
  void operator=(const vtkCaveSynchronizedRenderers&); // Not implemented
//ETX
};

#endif
