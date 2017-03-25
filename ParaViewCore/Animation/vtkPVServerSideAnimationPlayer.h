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

class VTKPVANIMATION_EXPORT vtkPVServerSideAnimationPlayer : public vtkObject
{
public:
  static vtkPVServerSideAnimationPlayer* New();
  vtkTypeMacro(vtkPVServerSideAnimationPlayer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  vtkSetStringMacro(SessionProxyManagerState);
  vtkSetStringMacro(FileName);

  /**
   * Call this method to setup the handlers to observer client being
   * disconnected from the server to save animation.
   */
  void Activate();

protected:
  vtkPVServerSideAnimationPlayer();
  virtual ~vtkPVServerSideAnimationPlayer();

  char* SessionProxyManagerState;
  char* FileName;

private:
  vtkPVServerSideAnimationPlayer(const vtkPVServerSideAnimationPlayer&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVServerSideAnimationPlayer&) VTK_DELETE_FUNCTION;
};

#endif
