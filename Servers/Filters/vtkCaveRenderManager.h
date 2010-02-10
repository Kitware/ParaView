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

#include "vtkIceTConstants.h"   // For constant definitions

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
  vtkTypeRevisionMacro(vtkCaveRenderManager, vtkParallelRenderManager);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual vtkRenderer *MakeRenderer();

  virtual void SetController(vtkMultiProcessController *controller);

  virtual void SetRenderWindow(vtkRenderWindow *renwin);

  void ComputeCamera(vtkCamera* cam);
  void SetNumberOfDisplays(int numberOfDisplays);
  void SetDisplay(double idx, double origin0, double origin1, double origin2, 
                              double x0, double x1, double x2,
                              double y0, double y1, double y2);
  void DefineDisplay(int idx, double origin[3], double x[3], double y[3]);  


protected:
  vtkCaveRenderManager();
  virtual ~vtkCaveRenderManager();

  virtual void CollectWindowInformation(vtkMultiProcessStream&);
  virtual bool ProcessWindowInformation(vtkMultiProcessStream&);

  virtual void CollectRendererInformation(vtkRenderer *, vtkMultiProcessStream&);
  virtual bool ProcessRendererInformation(vtkRenderer *, vtkMultiProcessStream&);

  virtual void PreRenderProcessing();
  virtual void PostRenderProcessing();

  int ContextDirty;
  vtkTimeStamp ContextUpdateTime;

  int    NumberOfDisplays;
  double **Displays;
  double DisplayOrigin[4];
  double DisplayX[4];
  double DisplayY[4];

private:
  vtkCaveRenderManager(const vtkCaveRenderManager&); // Not implemented
  void operator=(const vtkCaveRenderManager&); // Not implemented
};

#endif //__vtkCaveRenderManager_h
