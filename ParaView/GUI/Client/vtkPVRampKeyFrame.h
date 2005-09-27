/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRampKeyFrame.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRampKeyFrame - gui for ramp key frame. 
// .SECTION Description
//

#ifndef __vtkPVRampKeyFrame_h
#define __vtkPVRampKeyFrame_h

#include "vtkPVPropertyKeyFrame.h"

class VTK_EXPORT vtkPVRampKeyFrame : public vtkPVPropertyKeyFrame
{
public:
  static vtkPVRampKeyFrame* New();
  vtkTypeRevisionMacro(vtkPVRampKeyFrame, vtkPVPropertyKeyFrame);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPVRampKeyFrame();
  ~vtkPVRampKeyFrame();

  virtual void ChildCreate(vtkKWApplication* app);

private:
  vtkPVRampKeyFrame(const vtkPVRampKeyFrame&); // Not implemented.
  void operator=(const vtkPVRampKeyFrame&); // Not implemented.
  
};

#endif
