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

#include "vtkPVPropertyKeyFrame.h"

class vtkKWThumbWheel;
class vtkKWLabel;

class VTK_EXPORT vtkPVSinusoidKeyFrame : public vtkPVPropertyKeyFrame
{
public:
  static vtkPVSinusoidKeyFrame* New();
  vtkTypeRevisionMacro(vtkPVSinusoidKeyFrame, vtkPVPropertyKeyFrame);
  void PrintSelf(ostream& os, vtkIndent indent);

  void FrequencyChangedCallback();
  void PhaseChangedCallback();
  void OffsetChangedCallback();

  void SetFrequency(double base);
  void SetFrequencyWithTrace(double f);
  double GetFrequency();
  
  void SetPhase(double v);
  void SetPhaseWithTrace(double p);
  double GetPhase();
  
  void SetOffsetWithTrace(double o);
  void SetOffset(double v);
  double GetOffset();

  virtual void SaveState(ofstream* file);
  virtual void UpdateEnableState();
protected:
  vtkPVSinusoidKeyFrame();
  ~vtkPVSinusoidKeyFrame();

  virtual void ChildCreate();

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
