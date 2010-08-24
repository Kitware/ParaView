/*=========================================================================

  Program:   ParaView
  Module:    vtkCaveRenderManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#ifndef __vtkCaveRenderManager_h
#define __vtkCaveRenderManager_h

#include "vtkParallelRenderManager.h"
#include "vtkMatrix4x4.h"

class vtkIceTRenderer;
class vtkIntArray;
class vtkPerspectiveTransform;
class vtkPKdTree;
class vtkFloatArray;
class vtkCamera;

class VTK_EXPORT vtkCaveRenderManager : public vtkParallelRenderManager
{
public:
  static vtkCaveRenderManager *New();
  vtkTypeMacro(vtkCaveRenderManager, vtkParallelRenderManager);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual vtkRenderer *MakeRenderer();

  virtual void SetController(vtkMultiProcessController *controller);

  virtual void SetRenderWindow(vtkRenderWindow *renwin);

  void ComputeCameraNew(vtkCamera* cam);
  void ComputeCamera(vtkCamera* cam);
  void SetNumberOfDisplays(int numberOfDisplays);
  void SetDisplay(double idx, double origin0, double origin1, double origin2,
                              double x0, double x1, double x2,
                              double y0, double y1, double y2);
  void DefineDisplay(int idx, double origin[3], double x[3], double y[3]);

  // Description:
  // This method is used to configure the display at startup. The
  // display is only configurable if the head tracking is set. The
  // typical use case is a CAVE like VR setting and we would like the
  // head-tracked camera to be aware of the display in the room
  // coordinates.
  void SetDisplayConfig();

protected:
  vtkCaveRenderManager();
  virtual ~vtkCaveRenderManager();

  virtual void CollectWindowInformation(vtkMultiProcessStream&);
  virtual bool ProcessWindowInformation(vtkMultiProcessStream&);

  virtual void CollectRendererInformation(vtkRenderer *, vtkMultiProcessStream&);
  virtual bool ProcessRendererInformation(vtkRenderer *, vtkMultiProcessStream&);

  virtual void PreRenderProcessing();
  virtual void PostRenderProcessing();

  // Description:
  // This sets the SurfaceRot transfromation based on the screen
  // basis and the room basis
  void SetSurfaceRotation( double xBase[3], double yBase[3], double zBase[3],
                           double xRoom[3], double yRoom[3], double zRoom[3] );


  int ContextDirty;
  vtkTimeStamp ContextUpdateTime;

  int    NumberOfDisplays;

  double **Displays;
  double DisplayOrigin[4];
  double DisplayX[4];
  double DisplayY[4];

  // parms to send
  vtkMatrix4x4 *SurfaceRot;
  double O2Screen;
  double O2Right;
  double O2Left;
  double O2Top;
  double O2Bottom;

private:
  vtkCaveRenderManager(const vtkCaveRenderManager&); // Not implemented
  void operator=(const vtkCaveRenderManager&); // Not implemented
};

#endif //__vtkCaveRenderManager_h
