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
class vtkSMAnimationSceneImageWriter;

class VTK_EXPORT vtkSMServerSideAnimationPlayer : public vtkSMObject
{
public:
  static vtkSMServerSideAnimationPlayer* New();
  vtkTypeMacro(vtkSMServerSideAnimationPlayer, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the connection on whose disconnection, this cleaner
  // becomes active.
  vtkSetMacro(ConnectionID, vtkIdType);
  vtkGetMacro(ConnectionID, vtkIdType);

  // Description:
  // This is the writer which we are going to tell to write
  // the animation out once the client disconnects.
  void SetWriter(vtkSMAnimationSceneImageWriter* writer);
  vtkGetObjectMacro(Writer, vtkSMAnimationSceneImageWriter);
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
  vtkSMAnimationSceneImageWriter* Writer;

private:
  vtkSMServerSideAnimationPlayer(const vtkSMServerSideAnimationPlayer&); // Not implemented.
  void operator=(const vtkSMServerSideAnimationPlayer&); // Not implemented.

  friend class vtkSMServerSideAnimationPlayerObserver;
  vtkSMServerSideAnimationPlayerObserver* Observer;
//ETX
};

#endif

