/*=========================================================================

  Program:   ParaView
  Module:    vtkSequenceAnimationPlayer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSequenceAnimationPlayer
 *
 *
*/

#ifndef vtkSequenceAnimationPlayer_h
#define vtkSequenceAnimationPlayer_h

#include "vtkAnimationPlayer.h"
#include "vtkPVAnimationModule.h" // needed for export macro

class VTKPVANIMATION_EXPORT vtkSequenceAnimationPlayer : public vtkAnimationPlayer
{
public:
  static vtkSequenceAnimationPlayer* New();
  vtkTypeMacro(vtkSequenceAnimationPlayer, vtkAnimationPlayer);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  vtkSetClampMacro(NumberOfFrames, int, 2, VTK_INT_MAX);
  vtkGetMacro(NumberOfFrames, int);

protected:
  vtkSequenceAnimationPlayer();
  ~vtkSequenceAnimationPlayer() override;

  void StartLoop(double, double, double*) VTK_OVERRIDE;
  void EndLoop() VTK_OVERRIDE{};

  /**
   * Return the next time given the current time.
   */
  double GetNextTime(double currentime) VTK_OVERRIDE;

  double GoToNext(double start, double end, double currenttime) VTK_OVERRIDE;
  double GoToPrevious(double start, double end, double currenttime) VTK_OVERRIDE;

  int NumberOfFrames;
  int MaxFrameWindow;
  double StartTime;
  double EndTime;
  int FrameNo;

private:
  vtkSequenceAnimationPlayer(const vtkSequenceAnimationPlayer&) = delete;
  void operator=(const vtkSequenceAnimationPlayer&) = delete;
};

#endif
