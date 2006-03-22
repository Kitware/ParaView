/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLODRenderModuleProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMLODRenderModuleProxy -  Render module supporting LODs.
// .SECTION Description
// Mangages rendering and LODs.
// This class can be used alone when running serially. It handles the two 
// pipeline branches which render in parallel. Subclasses handle parallel 
// rendering. 

#ifndef __vtkSMLODRenderModuleProxy_h
#define __vtkSMLODRenderModuleProxy_h

#include "vtkSMSimpleRenderModuleProxy.h"
// We could have very well derrived this from vtkSMRenderModuleProxy, but hey!

class vtkSMDisplayProxy;
class vtkSMLODRenderModuleProxyObserver;
class VTK_EXPORT vtkSMLODRenderModuleProxy : public vtkSMSimpleRenderModuleProxy
{
public:
  static vtkSMLODRenderModuleProxy* New();
  vtkTypeRevisionMacro(vtkSMLODRenderModuleProxy, vtkSMSimpleRenderModuleProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Any display that must be rendered by this rendermodule
  // needs to be added to it. Overridden to add even listerns
  // to note when LOD Information of the Display changes.
  virtual void AddDisplay(vtkSMDisplayProxy* disp);
  virtual void RemoveDisplay(vtkSMDisplayProxy* disp);

  // Description
  // Subclass can create their own vtkSMDisplayProxy object by
  // implementing this method.
  // So far, others displays are not.
  virtual vtkSMDisplayProxy* CreateDisplayProxy();

  // Description:
  // Set the LOD Threshold.
  vtkSetMacro(LODThreshold, double);
  vtkGetMacro(LODThreshold, double);

  // Description:
  // Set the LOD resolution.
  void SetLODResolution(int);
  vtkGetMacro(LODResolution, int);

  virtual void InteractiveRender();
protected:
  vtkSMLODRenderModuleProxy();
  ~vtkSMLODRenderModuleProxy();
 
  vtkSMProxy* CollectProxy;
  vtkSMProxy* LODCollectProxy;
  double LODThreshold;
  int LODResolution;

  int TotalVisibleGeometryMemorySizeValid;
  unsigned long TotalVisibleGeometryMemorySize;
  vtkSetMacro(TotalVisibleGeometryMemorySizeValid, int);

  int TotalVisibleLODGeometryMemorySizeValid;
  vtkSetMacro(TotalVisibleLODGeometryMemorySizeValid, int);
  unsigned long TotalVisibleLODGeometryMemorySize;

  void ComputeTotalVisibleMemorySize();
  unsigned long GetTotalVisibleGeometryMemorySize();
  unsigned long GetTotalVisibleLODGeometryMemorySize();

  // Description:
  // Indicates if LOD must be used for current Interactive Render.
  // Assumes that UpdateAllDisplays() has been called before 
  // calling this method.
  int GetUseLODDecision();
  //BTX
  friend class vtkSMLODRenderModuleProxyObserver;
  vtkSMLODRenderModuleProxyObserver *Observer;
  //ETX
private:
  vtkSMLODRenderModuleProxy(const vtkSMLODRenderModuleProxy&); // Not implemented.
  void operator=(const vtkSMLODRenderModuleProxy&); // Not implemented.
};



#endif

