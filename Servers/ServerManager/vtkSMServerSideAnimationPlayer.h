/*=========================================================================

  Program:   ParaView
  Module:    vtkSMServerSideAnimationPlayer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMServerSideAnimationPlayer
// .SECTION Description
// This is an example of a server side connection cleaner. Connection cleaners
// are typically used on the server side to perform certain operations
// when a connection closes. This cleaner requires that the server-side
// server manager has been revived before the connection was terminated.
// It uses this revived server manager and render any render modules
// that may be present, and play any animation scenes that were
// present in the server manager.

#ifndef __vtkSMServerSideAnimationPlayer_h
#define __vtkSMServerSideAnimationPlayer_h

#include "vtkSMObject.h"

class vtkSMServerSideAnimationPlayerObserver;

class VTK_EXPORT vtkSMServerSideAnimationPlayer : public vtkSMObject
{
public:
  static vtkSMServerSideAnimationPlayer* New();
  vtkTypeRevisionMacro(vtkSMServerSideAnimationPlayer, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the connection on whose disconnection, this cleaner
  // becomes active.
  vtkSetMacro(ConnectionID, vtkIdType);
  vtkGetMacro(ConnectionID, vtkIdType);

  // Description:
  // These are properties that are needed to save an animation.
  // If AnimationFileName is set, then only this object will
  // attempt to save the animation.
  vtkSetStringMacro(AnimationFileName);
  vtkGetStringMacro(AnimationFileName);

  // Description:
  // These are properties that are needed to save an animation.
  vtkSetVector2Macro(Size, int);
  vtkGetVector2Macro(Size, int);

  // Description:
  // These are properties that are needed to save an animation.
  vtkSetMacro(FrameRate, double);
  vtkGetMacro(FrameRate, double);

  // Description:
  // These are properties that are needed to save an animation.
  vtkSetMacro(Quality, int);
  vtkGetMacro(Quality, int);
//BTX
protected:
  vtkSMServerSideAnimationPlayer();
  ~vtkSMServerSideAnimationPlayer();
  
  // Description:
  // Event handler.
  void ExecuteEvent(vtkObject* obj, unsigned long event, void* data);

  // Description:
  // Called the the connection pointer by ConnectionID is closed.
  virtual void PerformActions();

  vtkIdType ConnectionID;

  char* AnimationFileName;
  int Size[2];
  double FrameRate;
  int Quality;
private:
  vtkSMServerSideAnimationPlayer(const vtkSMServerSideAnimationPlayer&); // Not implemented.
  void operator=(const vtkSMServerSideAnimationPlayer&); // Not implemented.

  friend class vtkSMServerSideAnimationPlayerObserver;
  vtkSMServerSideAnimationPlayerObserver* Observer;
//ETX
};

#endif

