/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAnimationSceneWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMAnimationSceneWriter
 * @brief   helper class used
 * to write animations.
 *
 * vtkSMAnimationSceneWriter is an abstract superclass for writers
 * that can write animations out.
*/

#ifndef vtkSMAnimationSceneWriter_h
#define vtkSMAnimationSceneWriter_h

#include "vtkRemotingAnimationModule.h" //needed for exports
#include "vtkSMSessionObject.h"

class vtkSMAnimationScene;
class vtkSMProxy;

class VTKREMOTINGANIMATION_EXPORT vtkSMAnimationSceneWriter : public vtkSMSessionObject
{
public:
  vtkTypeMacro(vtkSMAnimationSceneWriter, vtkSMSessionObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Convenience method to set the proxy.
   */
  virtual void SetAnimationScene(vtkSMProxy*);

  //@{
  /**
   * Get/Set the animation scene that this writer will write.
   */
  virtual void SetAnimationScene(vtkSMAnimationScene*);
  vtkGetObjectMacro(AnimationScene, vtkSMAnimationScene);
  //@}

  /**
   * Begin the saving. This will result in playing of the animation.
   * Returns the status of the save.
   */
  bool Save();

  //@{
  /**
   * Get/Set the filename.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Get/Set the start file count.
   */
  vtkSetMacro(StartFileCount, int);
  vtkGetMacro(StartFileCount, int);
  //@}

  //@{
  /**
   * Get/Set time window that we want to write
   * If PlaybackTimeWindow[0] > PlaybackTimeWindow[1] that mean that we
   * want to export the full time range available.
   */
  vtkSetVector2Macro(PlaybackTimeWindow, double);
  vtkGetVector2Macro(PlaybackTimeWindow, double);
  //@}

protected:
  vtkSMAnimationSceneWriter();
  ~vtkSMAnimationSceneWriter() override;

  unsigned long ObserverID;
  vtkSMAnimationScene* AnimationScene;

  /**
   * Subclasses should override this method.
   * Called to initialize saving.
   */
  virtual bool SaveInitialize(int countStart) = 0;

  /**
   * Subclasses should override this method.
   * Called to save a particular frame.
   */
  virtual bool SaveFrame(double time) = 0;

  /**
   * Subclasses should override this method.
   * Called to finalize saving.
   */
  virtual bool SaveFinalize() = 0;

  void ExecuteEvent(vtkObject* caller, unsigned long eventid, void* calldata);

  // Flag indicating if we are currently saving.
  // Set on entering Save() and cleared before leaving Save().
  bool Saving;
  bool SaveFailed;
  char* FileName;
  double PlaybackTimeWindow[2];
  int StartFileCount;

private:
  vtkSMAnimationSceneWriter(const vtkSMAnimationSceneWriter&) = delete;
  void operator=(const vtkSMAnimationSceneWriter&) = delete;
};

#endif
