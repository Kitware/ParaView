/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBooleanKeyFrame.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVBooleanKeyFrame - gui for ramp key frame. 
// .SECTION Description
//

#ifndef __vtkPVBooleanKeyFrame_h
#define __vtkPVBooleanKeyFrame_h

#include "vtkPVKeyFrame.h"

class VTK_EXPORT vtkPVBooleanKeyFrame : public vtkPVKeyFrame
{
public:
  static vtkPVBooleanKeyFrame* New();
  vtkTypeRevisionMacro(vtkPVBooleanKeyFrame, vtkPVKeyFrame);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPVBooleanKeyFrame();
  ~vtkPVBooleanKeyFrame();

  virtual void ChildCreate(vtkKWApplication* app);

private:
  vtkPVBooleanKeyFrame(const vtkPVBooleanKeyFrame&); // Not implemented.
  void operator=(const vtkPVBooleanKeyFrame&); // Not implemented.
  
};

#endif
