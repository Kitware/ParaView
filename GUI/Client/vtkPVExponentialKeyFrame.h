/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExponentialKeyFrame.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVExponentialKeyFrame - gui for exponential key frame. 
// .SECTION Description
// GUI for exponential key frame.

#ifndef __vtkPVExponentialKeyFrame_h
#define __vtkPVExponentialKeyFrame_h

#include "vtkPVPropertyKeyFrame.h"

class vtkKWThumbWheel;
class vtkKWLabel;

class VTK_EXPORT vtkPVExponentialKeyFrame : public vtkPVPropertyKeyFrame
{
public:
  static vtkPVExponentialKeyFrame* New();
  vtkTypeRevisionMacro(vtkPVExponentialKeyFrame, vtkPVPropertyKeyFrame);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Callbacks for GUI
  void BaseChangedCallback();
  void StartPowerChangedCallback();
  void EndPowerChangedCallback();

  void SetBase(double base);
  void SetBaseWithTrace(double base);
  double GetBase();
  
  void SetStartPower(double v);
  void SetStartPowerWithTrace(double v);
  double GetStartPower();
  
  void SetEndPower(double v);
  void SetEndPowerWithTrace(double v);
  double GetEndPower();

  virtual void SaveState(ofstream* file);
  virtual void UpdateEnableState();

protected:
  vtkPVExponentialKeyFrame();
  ~vtkPVExponentialKeyFrame();

  virtual void ChildCreate(vtkKWApplication* app);

  vtkKWLabel* BaseLabel;
  vtkKWThumbWheel* BaseThumbWheel;

  vtkKWLabel* StartPowerLabel;
  vtkKWThumbWheel* StartPowerThumbWheel;

  vtkKWLabel* EndPowerLabel;
  vtkKWThumbWheel* EndPowerThumbWheel;

  virtual void UpdateValuesFromProxy();
private:
  vtkPVExponentialKeyFrame(const vtkPVExponentialKeyFrame&); // Not implemented.
  void operator=(const vtkPVExponentialKeyFrame&); // Not implemented.
  
};

#endif
