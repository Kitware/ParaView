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
// .NAME vtkSMAnimationSceneWriter - helper class used
// to write animations.
// .SECTION Description
// vtkSMAnimationSceneWriter is an abstract superclass for writers
// that can write animations out.

#ifndef __vtkSMAnimationSceneWriter_h
#define __vtkSMAnimationSceneWriter_h

#include "vtkSMSessionObject.h"

class vtkSMAnimationScene;
class vtkSMProxy;

class VTK_EXPORT vtkSMAnimationSceneWriter : public vtkSMSessionObject
{
public:
  vtkTypeMacro(vtkSMAnimationSceneWriter, vtkSMSessionObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Convenience method to set the proxy.
  virtual void SetAnimationScene(vtkSMProxy*);

  // Description:
  // Get/Set the animation scene that this writer will write.
  virtual void SetAnimationScene(vtkSMAnimationScene*);
  vtkGetObjectMacro(AnimationScene, vtkSMAnimationScene);

  // Description:
  // Begin the saving. This will result in playing of the animation.
  // Returns the status of the save.
  bool Save();

  // Description:
  // Get/Set the filename.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkSMAnimationSceneWriter();
  ~vtkSMAnimationSceneWriter();

  unsigned long ObserverID;
  vtkSMAnimationScene* AnimationScene;

  // Description:
  // Subclasses should override this method.
  // Called to initialize saving.
  virtual bool SaveInitialize() = 0;

  // Description:
  // Subclasses should override this method.
  // Called to save a particular frame.
  virtual bool SaveFrame(double time) = 0;

 // Description:
  // Subclasses should override this method.
  // Called to finalize saving.
  virtual bool SaveFinalize() = 0;

  void ExecuteEvent(vtkObject* caller, unsigned long eventid, void* calldata);

  // Flag indicating if we are currently saving.
  // Set on entering Save() and cleared before leaving Save().
  bool Saving;
  bool SaveFailed;
  char* FileName;
private:
  vtkSMAnimationSceneWriter(const vtkSMAnimationSceneWriter&); // Not implemented.
  void operator=(const vtkSMAnimationSceneWriter&); // Not implemented.
};

#endif


