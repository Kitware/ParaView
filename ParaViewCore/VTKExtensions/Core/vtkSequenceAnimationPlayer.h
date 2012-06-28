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
// .NAME vtkSequenceAnimationPlayer
// .SECTION Description
//

#ifndef __vtkSequenceAnimationPlayer_h
#define __vtkSequenceAnimationPlayer_h

#include "vtkAnimationPlayer.h"

class VTK_EXPORT vtkSequenceAnimationPlayer : public vtkAnimationPlayer
{
public:
  static vtkSequenceAnimationPlayer* New();
  vtkTypeMacro(vtkSequenceAnimationPlayer, vtkAnimationPlayer);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetClampMacro(NumberOfFrames, int, 2, VTK_INT_MAX);
  vtkGetMacro(NumberOfFrames, int);

//BTX
protected:
  vtkSequenceAnimationPlayer();
  ~vtkSequenceAnimationPlayer();

  virtual void StartLoop(double, double, double);
  virtual void EndLoop() {};

  // Description:
  // Return the next time given the current time.
  virtual double GetNextTime(double currentime);

  virtual double GoToNext(double start, double end, double currenttime);
  virtual double GoToPrevious(double start, double end, double currenttime);

  int NumberOfFrames;
  double StartTime;
  double EndTime;
  int FrameNo;
private:
  vtkSequenceAnimationPlayer(const vtkSequenceAnimationPlayer&); // Not implemented
  void operator=(const vtkSequenceAnimationPlayer&); // Not implemented
//ETX
};

#endif

