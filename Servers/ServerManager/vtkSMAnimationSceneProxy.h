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
// Proxy for animation scene. A scene is an animation setup that can be played.
// Also supports writing out animation images (movie) and animation geometry.
// Like all animation proxies, this is a client side proxy with not server 
// side VTK objects created.
// .SECTION See Also
// vtkAnimationScene vtkSMAnimationCueProxy

#ifndef __vtkSMAnimationSceneProxy_h
#define __vtkSMAnimationSceneProxy_h

#include "vtkSMAnimationCueProxy.h"

class vtkAnimationScene;
class vtkCollection;
class vtkCollectionIterator;
class vtkSMRenderModuleProxy;
class vtkImageWriter;
class vtkGenericMovieWriter;

class VTK_EXPORT vtkSMAnimationSceneProxy : public vtkSMAnimationCueProxy
{
public:
  static vtkSMAnimationSceneProxy* New();
  vtkTypeRevisionMacro(vtkSMAnimationSceneProxy, vtkSMAnimationCueProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Save the state of this proxy in the batch file.
  virtual void SaveInBatchScript(ofstream*);

  // Description:
  // Start playing the animation. On every \c Tick, 
  // \c vtkCommand::AnimationCueTickEvent is fired. One can call \c Stop()
  // in the event handler for this event to abort playing of the animation.
  // If \c Loop is set, the animation will be played in a loop.
  // This function returns only after the animation playing has stopped.
  void Play();

  // Description:
  // Stops playing the animation. This method has any effect only when
  // called within the vtkCommand::AnimationCueTickEvent event handler. 
  // This event is fired when playing the animation.
  void Stop();

  // Description:
  // Returns the status of the player. True when the animation is being played.
  int IsInPlay();
 
  // Description:
  // Set/Get if the animation should be played in a loop.
  void SetLoop(int loop);
  int GetLoop();

  // Description;
  // Set/Get the frame rate for the animation. Frame rate is used only when
  // the play mode is \c vtkAnimationScene::PLAYMODE_SEQUENCE.
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
 
  // Description:
  // Method to set the current time. This updates the proxies to reflect the state
  // at the indicated time.
  void SetAnimationTime(double time);

  // Description:
  // Saves the animation as a sequence of images or a movie file.
  // The method is not accessible using property interface.
  // Return 0 on success.
  int SaveImages(const char* fileRoot, const char* ext, int width, int height, double framerate, int quality);

  // Description:
  // Save the geometry of the animation.
  // Note that this method is not accessible using property interface.
  // Returns 0 on success.
  int SaveGeometry(const char* filename);

protected:
  vtkSMAnimationSceneProxy();
  ~vtkSMAnimationSceneProxy();

  // Called on every tick to save images.
  void SaveImages();

  // Called on every tick to save geometry.
  void SaveGeometry(double time);

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

  // Stuff for saving Animation Images.
  vtkImageWriter* ImageWriter;
  vtkGenericMovieWriter* MovieWriter;
  char* FileRoot;
  char* FileExtension;
  int FileCount;
  int SaveFailed;
  vtkSetStringMacro(FileRoot);
  vtkSetStringMacro(FileExtension);

  // Stuff for saving Geometry.
  vtkSMProxy *GeometryWriter;

private:
  vtkSMAnimationSceneProxy(const vtkSMAnimationSceneProxy&); // Not implemented.
  void operator=(const vtkSMAnimationSceneProxy&); // Not implemented.

  // Used to store the magnification of the saved image
  int Magnification;
  int InSaveAnimation;
};


#endif

