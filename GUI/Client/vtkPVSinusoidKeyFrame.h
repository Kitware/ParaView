/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSinusoidKeyFrame.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSinusoidKeyFrame - gui for sinusoid key frame. 
// .SECTION Description
//

#ifndef __vtkPVSinusoidKeyFrame_h
#define __vtkPVSinusoidKeyFrame_h

#include "vtkPVKeyFrame.h"

class vtkKWThumbWheel;
class vtkKWLabel;

class VTK_EXPORT vtkPVSinusoidKeyFrame : public vtkPVKeyFrame
{
public:
  static vtkPVSinusoidKeyFrame* New();
  vtkTypeRevisionMacro(vtkPVSinusoidKeyFrame, vtkPVKeyFrame);
  void PrintSelf(ostream& os, vtkIndent indent);

  void FrequencyChangedCallback();
  void PhaseChangedCallback();
  void OffsetChangedCallback();

  void SetFrequency(double base);
  double GetFrequency();
  
  void SetPhase(double v);
  double GetPhase();
  
  void SetOffset(double v);
  double GetOffset();

  virtual void SaveState(ofstream* file);
  virtual void UpdateEnableState();
protected:
  vtkPVSinusoidKeyFrame();
  ~vtkPVSinusoidKeyFrame();

  virtual void ChildCreate(vtkKWApplication* app);

  vtkKWLabel* PhaseLabel;
  vtkKWThumbWheel* PhaseThumbWheel;

  vtkKWLabel* FrequencyLabel;
  vtkKWThumbWheel* FrequencyThumbWheel;

  vtkKWLabel* OffsetLabel;
  vtkKWThumbWheel* OffsetThumbWheel;

  virtual void UpdateValuesFromProxy();
private:
  vtkPVSinusoidKeyFrame(const vtkPVSinusoidKeyFrame&); // Not implemented.
  void operator=(const vtkPVSinusoidKeyFrame&); // Not implemented.
  
};

#endif
