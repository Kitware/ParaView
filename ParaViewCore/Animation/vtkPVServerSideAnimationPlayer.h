
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
/**
 * @class   vtkPVServerSideAnimationPlayer
 * @brief   help class for server side animation
 * saving at disconnection time.
*/

#ifndef vtkPVServerSideAnimationPlayer_h
#define vtkPVServerSideAnimationPlayer_h

#include "vtkObject.h"
#include "vtkPVAnimationModule.h" //needed for exports

class vtkSMAnimationSceneImageWriter;
class vtkPVXMLElement;

class VTKPVANIMATION_EXPORT vtkPVServerSideAnimationPlayer : public vtkObject
{
public:
  static vtkPVServerSideAnimationPlayer* New();
  vtkTypeMacro(vtkPVServerSideAnimationPlayer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetWriter(vtkSMAnimationSceneImageWriter* writer);
  void SetSessionProxyManagerState(const char* xml_state);

protected:
  vtkPVServerSideAnimationPlayer();
  virtual ~vtkPVServerSideAnimationPlayer();

  /**
   * Callback that is used to trigger the execution of the animation writing.
   */
  void TriggerExecution();

private:
  vtkPVServerSideAnimationPlayer(const vtkPVServerSideAnimationPlayer&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVServerSideAnimationPlayer&) VTK_DELETE_FUNCTION;

  class vtkInternals;
  vtkInternals* Internals;
  friend class vtkInternals;
};

#endif
