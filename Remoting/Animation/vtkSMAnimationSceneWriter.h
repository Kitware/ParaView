// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMAnimationSceneWriter
 * @brief   helper class used
 * to write animations.
 *
 * vtkSMAnimationSceneWriter is an abstract superclass for writers
 * that can write animations out.
 *
 * If an animation scene is provided and WriteTimeSteps is true (default),
 * it is expected to play the animation scene and output each frames.
 * Otherwise (no animation scene or WriteTimeSteps to false), it outputs
 * only the current frame.
 *
 * The main work for a subclass is to implement SaveFrame, that will
 * be called sequencially for each required frame.
 */

#ifndef vtkSMAnimationSceneWriter_h
#define vtkSMAnimationSceneWriter_h

#include "vtkRemotingAnimationModule.h" //needed for exports
#include "vtkSMSessionObject.h"

class vtkSMAnimationScene;
class vtkSMExporterProxy;
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

  ///@{
  /**
   * Get/Set the animation scene that this writer will write.
   */
  virtual void SetAnimationScene(vtkSMAnimationScene*);
  vtkGetObjectMacro(AnimationScene, vtkSMAnimationScene);
  ///@}

  /**
   * Begin the saving.
   * If WriteTimeSteps is true, this will result in playing of the animation.
   * Otherwise it juste save the current frame.
   *
   * Returns the status of the save.
   */
  bool Save();

  /**
   * Enable the writing of all timesteps.
   * When false, only current scene time is considered.
   * Default is true.
   */
  vtkSetMacro(WriteTimeSteps, bool);
  vtkGetMacro(WriteTimeSteps, bool);
  vtkBooleanMacro(WriteTimeSteps, bool);

  ///@{
  /**
   * Get/Set the filename.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  ///@}

  ///@{
  /**
   * Get/Set the start file count.
   */
  vtkSetMacro(StartFileCount, int);
  vtkGetMacro(StartFileCount, int);
  ///@}

  ///@{
  /**
   * Get/Set time window that we want to write
   * If PlaybackTimeWindow[0] > PlaybackTimeWindow[1] that mean that we
   * want to export the full time range available.
   * This handles time values and not time steps.
   */
  vtkSetVector2Macro(PlaybackTimeWindow, double);
  vtkGetVector2Macro(PlaybackTimeWindow, double);
  ///@}

  ///@{
  /**
   * Use window to set PlaybackTimeWindow based on AnimationScene times.
   */
  void SetFrameWindow(int window[2]);
  void SetFrameWindow(int min, int max);
  ///@}

  ///@{
  /**
   * Set/Get The stride which is used to extract the next frame.
   * E.g. 1, 2, 3, would have stride = 1, while 1, 3, 5 would have stride = 2.
   *
   * Default value is 1.
   */
  vtkSetMacro(Stride, int);
  vtkGetMacro(Stride, int);
  ///@}

  /**
   * Set an exporter to use for single frame export.
   * Default is nullptr.
   */
  void SetFrameExporterDelegate(vtkSMExporterProxy* exporter);

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

  /**
   * Return the Exporter to use for a single frame export, if any.
   */
  vtkSMExporterProxy* GetFrameExporterDelegate();

  /**
   * Return true if the current configuration allows for animation export.
   * i.e. if WriteTimeSteps is true and AnimationScene is set.
   */
  bool AnimationEnabled();

  // Flag indicating if we are currently saving.
  // Set on entering Save() and cleared before leaving Save().
  bool Saving;
  bool SaveFailed;
  char* FileName;
  double PlaybackTimeWindow[2];
  int StartFileCount;
  int Stride = 1;

private:
  vtkSMAnimationSceneWriter(const vtkSMAnimationSceneWriter&) = delete;
  void operator=(const vtkSMAnimationSceneWriter&) = delete;

  bool WriteTimeSteps = true;

  vtkSMExporterProxy* FrameExporterDelegate = nullptr;
};

#endif
