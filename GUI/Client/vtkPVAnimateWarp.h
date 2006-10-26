/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimateWarp.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVAnimateWarp - A special source for warp / animate combination
// .SECTION Description
// This is a subclass of vtkPVSource that makes it easier / more convenient
// to animate a warp (vector).

#ifndef __vtkPVAnimateWarp_h
#define __vtkPVAnimateWarp_h

#include "vtkPVSource.h"

class vtkKWFrame;
class vtkKWLabel;
class vtkKWPushButton;

class VTK_EXPORT vtkPVAnimateWarp : public vtkPVSource
{
public:
  static vtkPVAnimateWarp* New();
  vtkTypeRevisionMacro(vtkPVAnimateWarp, vtkPVSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the properties object, called by InitializeProperties.
  virtual void CreateProperties();

  // Description:
  // UNUSUAL BEHAVIOR: Clicking one of the mode shape buttons calls Accept.
  // Bring up the animation frame, and select the ScaleFactor parameter.
  // If it has no keyframes, add one corresponding to the button clicked.
  void AnimateScaleFactor(int modeShape);

protected:
  vtkPVAnimateWarp();
  ~vtkPVAnimateWarp();

  vtkKWFrame      *AnimateFrame;
  vtkKWPushButton *RampButton;
  vtkKWPushButton *StepButton;
  vtkKWPushButton *ExponentialButton;
  vtkKWPushButton *SinusoidalButton;
  vtkKWLabel      *AnimateLabel;

private:
  vtkPVAnimateWarp(const vtkPVAnimateWarp&); // Not implemented
  void operator=(const vtkPVAnimateWarp&); // Not implemented
};

#endif

