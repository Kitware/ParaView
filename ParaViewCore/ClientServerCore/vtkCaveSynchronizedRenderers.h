/*=========================================================================

  Program:   ParaView
  Module:    vtkCaveSynchronizedRenderers.h

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

  // Description:
  // Set the eye separation for all the displays.
  void SetEyeSeparation(double eyeSeparation);

  void DefineDisplay(int idx, double origin[3], double x[3], double y[3]);


  // Description:
  // Method to update the camera.
  void ComputeCamera(vtkCamera* cam);

  double EyeSeparation;
  int    NumberOfDisplays;
  double **Displays;
  double DisplayOrigin[3];
  double DisplayX[3];
  double DisplayY[3];
  int once;

private:
  vtkCaveSynchronizedRenderers(const vtkCaveSynchronizedRenderers&); // Not implemented
  void operator=(const vtkCaveSynchronizedRenderers&); // Not implemented
//ETX
};

#endif
