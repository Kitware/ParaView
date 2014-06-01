
/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerSideAnimationPlayer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVServerSideAnimationPlayer - help class for server side animation
// saving at disconnection time.

#ifndef __vtkPVServerSideAnimationPlayer_h
#define __vtkPVServerSideAnimationPlayer_h

#include "vtkPVAnimationModule.h" //needed for exports
#include "vtkObject.h"

class vtkSMAnimationSceneImageWriter;
class vtkPVXMLElement;

class VTKPVANIMATION_EXPORT vtkPVServerSideAnimationPlayer : public vtkObject
{
public:
  static vtkPVServerSideAnimationPlayer* New();
  vtkTypeMacro(vtkPVServerSideAnimationPlayer,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetWriter(vtkSMAnimationSceneImageWriter* writer);
  void SetSessionProxyManagerState(const char* xml_state);

protected:
  vtkPVServerSideAnimationPlayer();
  virtual ~vtkPVServerSideAnimationPlayer();

  // Description:
  // Callback that is used to trigger the execution of the animation writing.
  void TriggerExecution();

private:
  vtkPVServerSideAnimationPlayer(const vtkPVServerSideAnimationPlayer&); // Not implemented
  void operator=(const vtkPVServerSideAnimationPlayer&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
  friend class vtkInternals;
};

#endif
