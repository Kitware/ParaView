/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAnimationSceneProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMAnimationSceneProxy - proxy for vtkAnimationScene
// .SECTION Description
// .SECTION See Also
// vtkSMProxy vtkSMAnimationCueProxy
//

#ifndef __vtkSMAnimationSceneProxy_h
#define __vtkSMAnimationSceneProxy_h

#include "vtkSMAnimationCueProxy.h"

class vtkAnimationScene;
class vtkCollection;
class vtkCollectionIterator;
class vtkSMRenderModuleProxy;

class VTK_EXPORT vtkSMAnimationSceneProxy : public vtkSMAnimationCueProxy
{
public:
  static vtkSMAnimationSceneProxy* New();
  vtkTypeRevisionMacro(vtkSMAnimationSceneProxy, vtkSMAnimationCueProxy);
  void PrintSelf(ostream& os, vtkIndent indent);


  virtual void SaveInBatchScript(ofstream*);

  void Play();
  void Stop();
  int IsInPlay();
  
  void SetLoop(int loop);
  int GetLoop();

  void SetFrameRate(double framerate);
  double GetFrameRate();

  // Description:
  // Note that when the play mode is set to Real Time, cacheing is
  // disabled.
  void SetPlayMode(int mode);
  int GetPlayMode();

  void AddCue(vtkSMProxy* cue);
  void RemoveCue(vtkSMProxy* cue);
 
  // Description:
  // Set if caching is enabled.
  // This method synchronizes the cahcing flag on every cue.
  virtual void SetCaching(int enable); 

  // Description:
  // This method calls InvalidateAllGeometries on the vtkSMRenderModuleProxy.
  // However, to minimize the calls to InvalidateAllGeometries, this call
  // keeps a flag indicating if CacheUpdate was ever called on the 
  // Render Module and calls InvalidateAllGeometries only of the flag
  // is set.
  void CleanCache();
  
  // Description:
  // Set the RenderModule Proxy.
  // Note that it is not reference counted.
  void SetRenderModuleProxy(vtkSMRenderModuleProxy* ren)
    { this->RenderModuleProxy = ren; } 
  
  void SetCurrentTime(double time);
protected:
  vtkSMAnimationSceneProxy();
  ~vtkSMAnimationSceneProxy();

  virtual void CreateVTKObjects(int numObjects);

  int GeometryCached; // flag indicating if this call asked RenderModuleProxy
    // to CacheUpdate.

  // Description:
  // Callbacks for corresponding Cue events. The argument must be 
  // casted to vtkAnimationCue::AnimationCueInfo.
  virtual void StartCueInternal(void* info);
  virtual void TickInternal(void* info);
  virtual void EndCueInternal(void* info);
  void CacheUpdate(void* info);
  
  vtkCollection* AnimationCueProxies;
  vtkCollectionIterator* AnimationCueProxiesIterator;

  vtkSMRenderModuleProxy* RenderModuleProxy;

private:
  vtkSMAnimationSceneProxy(const vtkSMAnimationSceneProxy&); // Not implemented.
  void operator=(const vtkSMAnimationSceneProxy&); // Not implemented.
};


#endif

