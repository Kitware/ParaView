/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLinearAnimationCueManipulatorProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMLinearAnimationCueManipulatorProxy
// .SECTION Description
// Linear value variance. An example of Manipulator which is not using KeyFrames.
// ParaView GUI never creates this. But provided as an example.
// .SECTION See Also
// vtkSMAnimationCueProxy 

#ifndef __vtkSMLinearAnimationCueManipulatorProxy_h
#define __vtkSMLinearAnimationCueManipulatorProxy_h

#include "vtkSMAnimationCueManipulatorProxy.h"

class vtkSMAnimationCueProxy;

class VTK_EXPORT vtkSMLinearAnimationCueManipulatorProxy : public vtkSMAnimationCueManipulatorProxy
{
public:
  vtkTypeMacro(vtkSMLinearAnimationCueManipulatorProxy, vtkSMAnimationCueManipulatorProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkSMLinearAnimationCueManipulatorProxy* New();

  // Get/Set the start values.
  vtkSetMacro(StartValue, double);
  vtkGetMacro(StartValue, double);

  vtkSetMacro(EndValue, double);
  vtkGetMacro(EndValue, double);

protected:
  vtkSMLinearAnimationCueManipulatorProxy();
  ~vtkSMLinearAnimationCueManipulatorProxy();

  double StartValue;
  double EndValue;

  // Description:
  // This updates the values based on current. 
  // currenttime is normalized to the time range of the Cue.
  virtual void UpdateValue(double currenttime, 
    vtkSMAnimationCueProxy* cueproxy);
private:
  vtkSMLinearAnimationCueManipulatorProxy(const vtkSMLinearAnimationCueManipulatorProxy&); // Not implemented.
  void operator=(const vtkSMLinearAnimationCueManipulatorProxy&); // Not implemented.
};


#endif

