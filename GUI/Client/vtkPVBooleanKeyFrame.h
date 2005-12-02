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

#include "vtkPVPropertyKeyFrame.h"

class VTK_EXPORT vtkPVBooleanKeyFrame : public vtkPVPropertyKeyFrame
{
public:
  static vtkPVBooleanKeyFrame* New();
  vtkTypeRevisionMacro(vtkPVBooleanKeyFrame, vtkPVPropertyKeyFrame);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPVBooleanKeyFrame();
  ~vtkPVBooleanKeyFrame();

  virtual void ChildCreate();

private:
  vtkPVBooleanKeyFrame(const vtkPVBooleanKeyFrame&); // Not implemented.
  void operator=(const vtkPVBooleanKeyFrame&); // Not implemented.
  
};

#endif
