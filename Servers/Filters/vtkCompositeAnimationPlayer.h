/*=========================================================================

  Program:   ParaView
  Module:    vtkCompositeAnimationPlayer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositeAnimationPlayer
// .SECTION Description
// This is composite animation player that can me made to play an animation
// using the active player. It provides API to add animation players and then
// set one of them as the active one.

#ifndef __vtkCompositeAnimationPlayer_h
#define __vtkCompositeAnimationPlayer_h

#include "vtkAnimationPlayer.h"

class VTK_EXPORT vtkCompositeAnimationPlayer : public vtkAnimationPlayer
{
public:
  static vtkCompositeAnimationPlayer* New();
  vtkTypeMacro(vtkCompositeAnimationPlayer, vtkAnimationPlayer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add an animation player. returns the index for the animation player.
  int AddPlayer(vtkAnimationPlayer*);

  // Description:
  // Removes all players.
  void RemoveAllPlayers();

  // Description:
  // Set the animation player at the given index as the active player.
  void SetActive(int index);

//BTX
protected:
  vtkCompositeAnimationPlayer();
  ~vtkCompositeAnimationPlayer();

  // Description:
  // Delegated to the active animation player.
  virtual void StartLoop(double starttime, double endtime, double currenttime);
  virtual void EndLoop();
  virtual double GetNextTime(double currentime);


  virtual double GoToNext(double start, double end, double currenttime);
  virtual double GoToPrevious(double start, double end, double currenttime);

private:
  vtkCompositeAnimationPlayer(const vtkCompositeAnimationPlayer&); // Not implemented
  void operator=(const vtkCompositeAnimationPlayer&); // Not implemented

  class vtkInternal;
  vtkInternal* Internal;

//ETX
};

#endif

